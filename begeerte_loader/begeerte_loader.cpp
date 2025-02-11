#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include <filesystem>
#include <tlhelp32.h>
#include <thread>
#include <chrono>
#include <random>
#include <ctime>
#include <map>
#include <urlmon.h>
#include <fstream>
#include <sstream>
#include <memory>

#include <print.h>

#pragma comment(lib, "urlmon.lib")

namespace fs = std::filesystem;

// 当前程序版本号（用于与云端 update.log 对比）
std::string currentVersion = "1.0.2";

// 全局变量：记录下载并重命名后的 DLL 文件路径，供清理时使用
std::string g_dllFilePath = "";

// 最大等待目标进程启动时间（秒）与额外注入延时（秒）
const int MAX_TIMEOUT_SECONDS = 300;
const int EXTRA_DELAY_SECONDS = 3;

/*
 * shutdown()
 * -----------
 * 任务结束时调用：
 * 1. 退出控制台窗口（调用 FreeConsole()），但进程继续运行；
 * 2. 每秒尝试删除全局记录的 DLL 文件（g_dllFilePath），直到删除成功后，调用 ExitProcess(0) 完全退出。
 */
void shutdown() {
    // 退出控制台窗口，隐藏输出窗口
    FreeConsole();

    // 循环每秒检查并尝试删除 DLL 文件
    while (!g_dllFilePath.empty() && fs::exists(g_dllFilePath)) {
        if (DeleteFileA(g_dllFilePath.c_str())) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    ExitProcess(0);
}

/*
 * generateRandomFileName()
 * ------------------------
 * 生成指定长度的随机文件名字符串（不含扩展名）。
 */
std::string generateRandomFileName(size_t length) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result;
    // 使用 static 随机引擎，避免同一程序运行中多次种子重复问题
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, chars.size() - 1);
    for (size_t i = 0; i < length; ++i) {
        result += chars[dist(rng)];
    }
    return result;
}

/*
 * SetRandomFileModificationDate()
 * ---------------------------------
 * 为指定文件随机设置“修改日期”。
 * 生成的随机时间范围为：2000-01-01 至 2020-12-31。
 */
bool SetRandomFileModificationDate(const fs::path& filePath) {
    try {
        // 构造下界时间：2000-01-01
        tm lower_tm = {};
        lower_tm.tm_year = 470;   // 正确设置年份为 2000
        lower_tm.tm_mon = 0;           // 1 月（从 0 开始）
        lower_tm.tm_mday = 1;
        time_t lower_bound = mktime(&lower_tm);

        // 构造上界时间：2020-12-31
        tm upper_tm = {};
        upper_tm.tm_year = 490;       // 正确设置年份为 2020
        upper_tm.tm_mon = 11;          // 12 月
        upper_tm.tm_mday = 31;
        time_t upper_bound = mktime(&upper_tm);

        // 生成随机时间（秒级）
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<time_t> dist(lower_bound, upper_bound);
        time_t randomTime = dist(rng);

        // 将 time_t 转换为 std::chrono::system_clock::time_point
        auto sctp = std::chrono::system_clock::from_time_t(randomTime);
        // 将 system_clock::time_point 转换为 fs::file_time_type
        auto ftime = fs::file_time_type(std::chrono::duration_cast<fs::file_time_type::duration>(
            sctp.time_since_epoch()));
        fs::last_write_time(filePath, ftime);
    }
    catch (const std::exception& ex) {
        std::cerr << "设置文件修改日期失败: " << ex.what() << std::endl;
        return false;
    }
    return true;
}

/*
 * ListProcesses()
 * ---------------
 * 枚举当前系统中所有进程，返回一个包含 (进程ID, 进程名称) 对的 vector。
 */
std::vector<std::pair<DWORD, std::string>> ListProcesses() {
    std::vector<std::pair<DWORD, std::string>> processes;
    PROCESSENTRY32 pe32{};
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // 获取系统进程快照
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return processes;
    }

    // 枚举所有进程
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            std::wstring ws(pe32.szExeFile);
            std::string processName(ws.begin(), ws.end());
            processes.emplace_back(pe32.th32ProcessID, processName);
        } while (Process32Next(hProcessSnap, &pe32));
    }
    CloseHandle(hProcessSnap);
    return processes;
}

/*
 * InjectDll()
 * -----------
 * 将指定 DLL（以文件路径形式传入）注入到目标进程中。
 * 具体步骤：
 * 1. 打开目标进程；
 * 2. 在目标进程中分配内存并写入 DLL 路径字符串；
 * 3. 获取 LoadLibraryA 地址，并创建远程线程调用它；
 * 4. 等待线程执行完毕并清理分配的内存。
 */
bool InjectDll(DWORD processID, const std::string& dllPath) {
    // 打开目标进程
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (!hProcess) {
        print("无法打开目标进程。");
        return false;
    }

    // 在目标进程中分配内存，存储 DLL 路径字符串
    LPVOID remoteMemory = VirtualAllocEx(hProcess, nullptr, dllPath.size() + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!remoteMemory) {
        print("无法在目标进程中分配内存。");
        CloseHandle(hProcess);
        return false;
    }

    // 将 DLL 路径写入目标进程内存
    if (!WriteProcessMemory(hProcess, remoteMemory, dllPath.c_str(), dllPath.size() + 1, nullptr)) {
        print("无法将 DLL 路径写入目标进程。");
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 获取 kernel32.dll 模块句柄（无需释放）
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        print("无法获取 kernel32.dll 模块句柄。");
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    LPVOID loadLibraryAddr = reinterpret_cast<LPVOID>(GetProcAddress(hKernel32, "LoadLibraryA"));
    if (!loadLibraryAddr) {
        print("无法获取 LoadLibraryA 的地址。");
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 在目标进程中创建远程线程，执行 LoadLibraryA
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryAddr),
        remoteMemory, 0, nullptr);
    if (!hThread) {
        print("无法在目标进程中创建远程线程。");
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    WaitForSingleObject(hThread, INFINITE);

    // 清理分配的内存和句柄
    VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    return true;
}

/*
 * checkBlacklistedProcesses()
 * -----------------------------
 * 检查是否存在指定游戏对应的黑名单进程。
 * 如果检测到黑名单进程，则输出警告并调用 shutdown() 结束任务。
 */
void checkBlacklistedProcesses(const std::map<std::string, std::vector<std::string>>& blacklistedProcesses,
    const std::string& selectedGame) {
    auto it = blacklistedProcesses.find(selectedGame);
    if (it == blacklistedProcesses.end())
        return;
    auto processes = ListProcesses();
    for (const auto& process : processes) {
        for (const auto& blkProc : it->second) {
            if (process.second == blkProc) {
                print("警告：检测到黑名单进程 (", blkProc, ")");
                shutdown();
            }
        }
    }
}

/*
 * DownloadFile()
 * -------------
 * 利用 URLDownloadToFileA 从远程服务器下载文件到本地指定路径。
 * 注意：仅输出下载过程提示信息，不显示远程服务器地址或 IP 信息。
 */
bool DownloadFile(const std::string& remoteUrl, const std::string& localPath) {
    print("开始下载文件: ", localPath, " ...");
    HRESULT hr = URLDownloadToFileA(nullptr, remoteUrl.c_str(), localPath.c_str(), 0, nullptr);
    if (hr == S_OK) {
        print("文件下载成功: ", localPath);
        return true;
    }
    else {
        print("文件下载失败: ", localPath, "，HRESULT：", hr);
        return false;
    }
}

/*
 * downloadTextFileContentFromUrl()
 * ------------------------
 * 直接从 URL 读取文本文件内容，不保存到本地文件。
 * 使用 URLOpenBlockingStream 打开网络流，并读取内容到字符串。
 */
std::string downloadTextFileContentFromUrl(const std::string& fileUrl) {
    std::string content;
    IStream* pStream = nullptr;
    HRESULT hr;

    // UTF-8 转 UTF-16
    int wideStrLen = MultiByteToWideChar(CP_UTF8, 0, fileUrl.c_str(), -1, nullptr, 0);
    if (wideStrLen == 0) {
        print("MultiByteToWideChar (获取长度) 失败，错误代码: ", GetLastError());
        return "";
    }
    std::unique_ptr<wchar_t[]> wideBuffer(new wchar_t[wideStrLen]);
    MultiByteToWideChar(CP_UTF8, 0, fileUrl.c_str(), -1, wideBuffer.get(), wideStrLen);
    std::wstring wideFileUrl(wideBuffer.get());

    hr = URLOpenBlockingStream(nullptr, wideFileUrl.c_str(), &pStream, 0, nullptr);
    if (SUCCEEDED(hr)) {
        print("成功打开网络流: ", fileUrl);
        constexpr int bufferSize = 1024;
        char buffer[bufferSize];
        ULONG bytesRead = 0;
        while (SUCCEEDED(pStream->Read(buffer, bufferSize - 1, &bytesRead)) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            content.append(buffer);
        }
        pStream->Release();

        // 移除 UTF-8 BOM（如果存在）
        if (content.size() >= 3 &&
            static_cast<unsigned char>(content[0]) == 0xEF &&
            static_cast<unsigned char>(content[1]) == 0xBB &&
            static_cast<unsigned char>(content[2]) == 0xBF) {
            content = content.substr(3);
        }
        // 去除首尾空白字符
        content.erase(0, content.find_first_not_of(" \t\r\n"));
        content.erase(content.find_last_not_of(" \t\r\n") + 1);
    }
    else {
        print("无法打开网络流: ", fileUrl, "，HRESULT: ", hr);
    }
    return content;
}

/*
 * readLogContent()
 * ----------------
 * 读取 update.log 文件的内容，返回版本号。
 * 修改为直接从 URL 读取，不保存到本地文件。
 */
std::string readLogContent(const std::string& logUrl) {
    return downloadTextFileContentFromUrl(logUrl);
}

/*
 * waitForTargetProcess()
 * ------------------------
 * 等待指定目标进程启动，最多等待 timeoutSeconds 秒。
 * 如果找到目标进程，则返回其进程 ID；否则调用 shutdown() 结束任务。
 */
DWORD waitForTargetProcess(const std::string& targetProcessName, int timeoutSeconds) {
    auto startTime = std::chrono::steady_clock::now();
    while (true) {
        auto processes = ListProcesses();
        for (const auto& proc : processes) {
            if (proc.second == targetProcessName) {
                print("找到目标进程 ", targetProcessName, "，进程 ID: ", proc.first);
                return proc.first;
            }
        }
        int elapsedSeconds = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime).count());
        int remainingSeconds = timeoutSeconds - elapsedSeconds;
        if (remainingSeconds <= 0) {
            print("超时！在 ", timeoutSeconds, " 秒内未找到目标进程。");
            shutdown();
        }
        print("等待游戏启动，剩余时间: ", remainingSeconds, " 秒...");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

/*
 * executeDelayedRenameAndLaunch()
 * -----------------------------
 * 替换子进程执行延迟重命名和启动新版本程序的操作。
 */
bool executeDelayedRenameAndLaunch() {
    char currentExePathBuffer[MAX_PATH] = { 0 };
    if (!GetModuleFileNameA(nullptr, currentExePathBuffer, MAX_PATH)) {
        print("获取自身程序路径失败，无法执行自身替换。");
        return false;
    }
    std::string currentExePathStr(currentExePathBuffer);
    fs::path currentExePath(currentExePathStr);

    const std::string originalExeName = "begeerte_loader.exe";
    fs::path originalExePath = fs::current_path() / originalExeName;

    print("替换子进程开始执行延迟...");
    print("  当前程序路径 (临时版本): ", currentExePathStr);
    print("  原始程序路径 (将被替换): ", originalExePath.string());
    print("  !!!  注意：本次更新为无备份模式，替换失败将不会自动恢复旧版本 !!!");
    print("  !!!  跳过备份旧版本程序步骤 (无备份模式) !!!");

    print("  等待文件锁释放...");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (fs::exists(originalExePath)) {
        print("目标文件已存在，准备删除...");
        std::error_code ec;
        if (!fs::remove(originalExePath, ec)) {
            print("  删除目标文件失败! 错误代码: ", ec.value(), ", 错误信息: ", ec.message());
            return false;
        }
        print("  目标文件已成功删除。");
    }

    if (!MoveFileA(currentExePath.string().c_str(), originalExePath.string().c_str())) {
        DWORD errorCode = GetLastError();
        print("  重命名自身程序 (替换) 失败! 错误代码: ", errorCode, ", 错误信息: 文件重命名操作失败。");
        print("  自身替换失败。");
        print("  !!!  警告：由于是无备份模式，程序可能处于不一致状态，请手动检查!");
        return false;
    }
    print("  自身程序重命名 (替换) 成功: ", originalExeName);

    // 为新替换后的 EXE 文件设置随机修改日期
    if (!SetRandomFileModificationDate(originalExePath)) {
        print("为重命名的 EXE 文件设置随机修改日期失败。");
    }

    print("  准备启动新版本程序: ", originalExeName);
    ShellExecuteA(NULL, "open", originalExeName.c_str(), NULL, NULL, SW_SHOW);
    print("  已启动新版本程序。");

    print("  替换子进程执行完毕，即将退出...");
    return true;
}

/*
 * launchRenameProcess()
 * -----------------------------
 * 启动自身的另一个进程，并传递 "__RENAME_PROCESS__" 参数，
 * 用于在独立的子进程中执行延迟重命名和启动操作。
 */
bool launchRenameProcess() {
    char currentExePathBuffer[MAX_PATH] = { 0 };
    if (!GetModuleFileNameA(nullptr, currentExePathBuffer, MAX_PATH)) {
        print("获取自身程序路径失败，无法启动替换子进程。");
        return false;
    }
    std::string currentExePath(currentExePathBuffer);

    print("准备启动替换子进程...");
    std::string currentDirectory = fs::current_path().string();
    SHELLEXECUTEINFOA sei = { sizeof(SHELLEXECUTEINFOA) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = NULL;
    sei.lpVerb = "open";
    sei.lpFile = currentExePath.c_str();
    sei.lpParameters = "__RENAME_PROCESS__";
    sei.lpDirectory = currentDirectory.c_str();
    sei.nShow = SW_HIDE;
    sei.hInstApp = NULL;

    if (!ShellExecuteExA(&sei)) {
        DWORD errorCode = GetLastError();
        print("启动替换子进程失败! 错误代码: ", errorCode, ", 错误信息: 调用 ShellExecuteEx 失败。");
        return false;
    }
    print("替换子进程已成功启动。");
    return true;
}

/*
 * mainEntry()
 * ------
 * 程序主入口：
 * 1. 检查命令行参数判断是否为替换子进程；
 * 2. 执行自动更新检查，并根据情况启动新版本程序；
 * 3. 重命名自身 EXE 文件（防检测）并随机设置修改日期；
 * 4. 根据用户选择确定目标游戏及对应参数，检查黑名单进程，并等待目标进程启动；
 * 5. 下载 DLL 文件，随机重命名后随机设置修改日期，并执行 DLL 注入；
 * 6. 注入任务结束后调用 shutdown() 退出程序。
 */
int mainEntry(int argc, char* argv[]) {
    char currentExePathBuffer[MAX_PATH] = { 0 };

    // 检查是否以 "__RENAME_PROCESS__" 参数启动（子进程模式）
    bool isRenameProcess = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "__RENAME_PROCESS__") {
            isRenameProcess = true;
            break;
        }
    }

    if (isRenameProcess) {
        print("程序以替换子进程模式启动，准备执行延迟重命名和启动...");
        executeDelayedRenameAndLaunch();
        return 0;
    }

    // 非替换子进程模式：检查自身是否为临时版本
    GetModuleFileNameA(nullptr, currentExePathBuffer, MAX_PATH);
    std::string currentExePath(currentExePathBuffer);
    fs::path exeFileName = fs::path(currentExePath).filename();
    const std::string tempExeNameHint = "begeerte_loader_1.exe";
    if (exeFileName.string() == tempExeNameHint) {
        print("程序以临时版本 ", tempExeNameHint, " 启动，准备启动替换子进程...");
        if (launchRenameProcess()) {
            print("替换子进程已启动，父进程即将退出。");
            return 0;
        }
        else {
            print("启动替换子进程失败，但将继续尝试以临时版本运行 (可能功能不完整).");
        }
    }

    // 设置控制台标题并提示用户
    SetConsoleTitleW(L"github.com/zetsr/begeerte");
    print("请勿重命名DLL文件！！！");

    // 自动更新检查
    const std::string remoteServerUrl = "https://zetsr.github.io/begeerte_/";
    std::string updateLogUrl = remoteServerUrl + "update.log";
    std::string newExeUrl = remoteServerUrl + "begeerte_loader.exe";
    std::string localNewExePath = (fs::current_path() / "begeerte_loader_1.exe").string();

    print("检查更新...");
    std::string serverVersion = readLogContent(updateLogUrl);
    if (!serverVersion.empty() && serverVersion != currentVersion) {
        print("发现新版本: ", serverVersion, ", 当前版本: ", currentVersion);
        print("准备下载更新...");
        if (DownloadFile(newExeUrl, localNewExePath)) {
            print("更新下载完成，准备启动新版本...");
            ShellExecuteA(NULL, "open", localNewExePath.c_str(), NULL, NULL, SW_SHOW);
            print("正在启动新版本 (临时版本), 旧版本即将退出...");
            return 0;
        }
        else {
            print("下载更新失败，将继续运行当前版本。");
        }
    }
    else {
        print("当前已是最新版本，无需更新。");
    }

    // 重命名自身 EXE 文件（防检测）：生成随机文件名，并将当前 EXE 移动为新名称
    char currentExePathRename[MAX_PATH] = { 0 };
    GetModuleFileNameA(nullptr, currentExePathRename, MAX_PATH);
    std::string currentExeRename(currentExePathRename);
    std::string newExeName = generateRandomFileName(10) + ".exe";
    std::string newExePath = (fs::current_path() / newExeName).string();
    if (!MoveFileA(currentExeRename.c_str(), newExePath.c_str())) {
        print("重命名程序失败。");
        return 1;
    }
    // 为新重命名的 EXE 文件设置随机修改日期
    if (!SetRandomFileModificationDate(fs::path(newExePath))) {
        print("为重命名的 EXE 文件设置随机修改日期失败。");
    }

    // 定义游戏与对应目标进程名称及 DLL 名称的映射关系
    std::map<std::string, std::pair<std::string, std::string>> gameToProcessAndDll = {
        { "Counter-Strike 2", { "cs2.exe", "begeerte_cs2" } },
        { "Day of Dragons", { "Dragons-Win64-Shipping.exe", "begeerte_dod" } },
        { "Day of Dragons [Test]", { "Dragons-Win64-Test.exe", "begeerte_dod_test" } },
        { "Draconia", { "Draconia-Win64-Shipping.exe", "begeerte_draconia" } },
        { "Dragons Legacy", { "Dragons-Win64-Shipping.exe", "begeerte_dl" } },
        { "Golden Treasure: The Great Green", { "Golden Treasure - The Great Green.exe", "begeerte_gttgg" } }
    };

    // 定义每个游戏对应的黑名单进程名称（如果存在）
    std::map<std::string, std::vector<std::string>> blacklistedProcesses = {
        { "Counter-Strike 2", {} },
        { "Day of Dragons", { "EasyAntiCheat_EOS.exe" } },
        { "Day of Dragons [Test]", { "EasyAntiCheat_EOS.exe" } },
        { "Draconia", {} },
        { "Dragons Legacy", {} },
        { "Golden Treasure: The Great Green", {} }
    };

    // 输出游戏选项，提示用户进行选择
    std::cout << "请选择游戏:" << std::endl;
    int index = 1;
    for (const auto& entry : gameToProcessAndDll) {
        print(index++, ". ", entry.first);
    }

    int gameChoice = 0;
    std::cin >> gameChoice;
    if (gameChoice < 1 || gameChoice > static_cast<int>(gameToProcessAndDll.size())) {
        print("无效的游戏选择。");
        return 1;
    }

    // 根据用户选择确定目标游戏及相关参数
    auto selectedGameIt = gameToProcessAndDll.begin();
    std::advance(selectedGameIt, gameChoice - 1);
    std::string targetProcessName = selectedGameIt->second.first;
    std::string targetDllName = selectedGameIt->second.second;

    // 检查是否存在黑名单进程（如有则立即结束任务）
    checkBlacklistedProcesses(blacklistedProcesses, selectedGameIt->first);

    // 等待目标进程启动（最多等待 MAX_TIMEOUT_SECONDS 秒），并获取目标进程 ID
    DWORD targetProcessID = waitForTargetProcess(targetProcessName, MAX_TIMEOUT_SECONDS);
    std::this_thread::sleep_for(std::chrono::seconds(EXTRA_DELAY_SECONDS));

    // 开始下载 DLL 文件
    const std::string remoteServerUrl_dll = "https://zetsr.github.io/begeerte_/";
    std::string remoteDllUrl = remoteServerUrl_dll + targetDllName + ".dll";
    std::string localDllPath = (fs::current_path() / (targetDllName + ".dll")).string();

    if (!DownloadFile(remoteDllUrl, localDllPath)) {
        print("未能成功下载 DLL 文件。");
        shutdown();
    }

    // 下载成功后，使用随机文件名重命名 DLL 文件，并更新全局变量 g_dllFilePath 供后续删除
    std::string randomDllName = generateRandomFileName(10) + ".dll";
    std::string randomDllPath = (fs::current_path() / randomDllName).string();
    if (!MoveFileA(localDllPath.c_str(), randomDllPath.c_str())) {
        g_dllFilePath = localDllPath;
        print("重命名 DLL 文件失败。");
        shutdown();
    }
    // 为重命名的 DLL 文件设置随机修改日期
    if (!SetRandomFileModificationDate(fs::path(randomDllPath))) {
        print("为重命名的 DLL 文件设置随机修改日期失败。");
    }
    g_dllFilePath = randomDllPath;
    print("DLL 文件已重命名为: ", randomDllName);

    // 执行 DLL 注入操作
    if (InjectDll(targetProcessID, randomDllPath)) {
        print("DLL 注入成功！");
    }
    else {
        print("DLL 注入失败。");
    }

    // 注入任务结束后，调用 shutdown() 退出程序（并循环尝试删除 DLL 文件）
    shutdown();
    return 0;
}

// 为了兼容性，main 函数调用 mainEntry()
int main(int argc, char* argv[]) {
    return mainEntry(argc, argv);
}
