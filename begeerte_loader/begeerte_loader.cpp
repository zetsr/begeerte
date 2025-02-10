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

// 版本号，用于和云端 update.log 对比
std::string currentVersion = "1.0.0"; // 请设置您当前的程序版本号

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
            // DLL 删除成功，不再输出（因为控制台已退出）
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
    std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<size_t> dist(0, chars.size() - 1);
    for (size_t i = 0; i < length; ++i) {
        result += chars[dist(rng)];
    }
    return result;
}

/*
 * ListProcesses()
 * ---------------
 * 枚举当前系统中所有进程，返回一个包含 (进程ID, 进程名称) 对的 vector。
 */
std::vector<std::pair<DWORD, std::string>> ListProcesses() {
    std::vector<std::pair<DWORD, std::string>> processes;
    PROCESSENTRY32 pe32 = { 0 };
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
            processes.push_back({ pe32.th32ProcessID, processName });
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

    // 分配目标进程内存，存储 DLL 路径字符串
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

    // 获取 LoadLibraryA 的地址
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        print("无法获取 kernel32.dll 模块句柄。");
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    LPVOID loadLibraryAddr = (LPVOID)GetProcAddress(hKernel32, "LoadLibraryA");
    if (!loadLibraryAddr) {
        print("无法获取 LoadLibraryA 的地址。");
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        FreeLibrary(hKernel32);
        return false;
    }
    FreeLibrary(hKernel32);


    // 在目标进程中创建远程线程，执行 LoadLibraryA
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
        (LPTHREAD_START_ROUTINE)loadLibraryAddr,
        remoteMemory, 0, nullptr);
    if (!hThread) {
        print("无法在目标进程中创建远程线程。");
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    WaitForSingleObject(hThread, INFINITE);

    // 清理内存和句柄
    VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
    if (hThread) {
        CloseHandle(hThread);
    }
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
    if (it == blacklistedProcesses.end()) {
        return; // 无黑名单进程
    }
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
 * 此函数已通用化，不仅限于 DLL 文件下载。
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
 *  直接从 URL 读取文本文件内容，不保存到本地文件。
 *  使用 URLOpenBlockingStream 打开网络流，并读取内容到字符串。
 */
std::string downloadTextFileContentFromUrl(const std::string& fileUrl) {
    std::string content = "";
    IStream* pStream = nullptr;
    HRESULT hr;

    // *** 使用 MultiByteToWideChar 进行 UTF-8 到 UTF-16 转换 ***
    int wideStrLen = MultiByteToWideChar(CP_UTF8, 0, fileUrl.c_str(), -1, nullptr, 0);
    if (wideStrLen == 0) {
        print("MultiByteToWideChar (获取长度) 失败，错误代码: ", GetLastError());
        return "";
    }
    std::unique_ptr<wchar_t[]> wideBuffer(new wchar_t[wideStrLen]);
    MultiByteToWideChar(CP_UTF8, 0, fileUrl.c_str(), -1, wideBuffer.get(), wideStrLen);
    std::wstring wideFileUrl = wideBuffer.get();


    hr = URLOpenBlockingStream(nullptr, wideFileUrl.c_str(), &pStream, 0, nullptr);
    if (SUCCEEDED(hr)) {
        print("成功打开网络流: ", fileUrl);
        constexpr int bufferSize = 1024;
        char buffer[bufferSize];
        ULONG bytesRead;
        while (true) {
            hr = pStream->Read(buffer, bufferSize - 1, &bytesRead);
            if (FAILED(hr) || bytesRead == 0) {
                break; // 读取错误或文件结束
            }
            buffer[bytesRead] = '\0';
            content += buffer;
        }
        pStream->Release();

        // 移除 UTF-8 BOM (如果存在)
        if (!content.empty() && static_cast<unsigned char>(content[0]) == 0xEF && static_cast<unsigned char>(content[1]) == 0xBB && static_cast<unsigned char>(content[2]) == 0xBF) {
            content = content.substr(3);
        }
        // 移除首尾空白字符
        size_t firstNonSpace = content.find_first_not_of(" \t\r\n");
        if (std::string::npos != firstNonSpace) {
            content = content.substr(firstNonSpace);
        }
        size_t lastNonSpace = content.find_last_not_of(" \t\r\n");
        if (std::string::npos != lastNonSpace) {
            content = content.substr(0, lastNonSpace + 1);
        }


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
 *  !!!  修改为直接从 URL 读取，不保存本地文件  !!!
 */
std::string readLogContent(const std::string& logUrl) {
    return downloadTextFileContentFromUrl(logUrl); // 仅调用网络读取函数，完全移除本地文件操作
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
        // 计算剩余等待时间
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
 *  !!!  此函数为替换子进程 (begeerte_loader_1.exe __RENAME_PROCESS__)  执行  !!!
 *  执行延迟等待, 自身重命名, 启动新版本 begeerte_loader.exe 操作
 */
bool executeDelayedRenameAndLaunch() {
    char currentExePathBuffer[MAX_PATH] = { 0 };
    if (!GetModuleFileNameA(nullptr, currentExePathBuffer, MAX_PATH)) {
        print("获取自身程序路径失败，无法执行自身替换。");
        return false;
    }
    std::string currentExePath_temp = currentExePathBuffer; // 当前程序 (临时版本，例如 begeerte_loader_1.exe)
    fs::path currentExePath(currentExePath_temp);

    std::string originalExeName = "begeerte_loader.exe"; // 原始程序名
    std::string originalExePath_str = fs::current_path().string() + "\\" + originalExeName;
    fs::path originalExePath(originalExePath_str);

    print("替换子进程开始执行延迟...");
    print("  当前程序路径 (临时版本): ", currentExePath_temp);
    print("  原始程序路径 (将被替换): ", originalExePath_str);
    print("  !!!  注意：本次更新为无备份模式，替换失败将不会自动恢复旧版本 !!!"); //  !!!  重要提示 !!!

    // 1.  !!!  移除备份步骤 !!!  不再备份旧版本程序步骤 (无备份模式) !!!
    print("  !!!  跳过备份旧版本程序步骤 (无备份模式) !!!");

    // 2. 延迟等待
    print("  等待文件锁释放...");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); //  增加 1 秒延迟

    // 3. 如果目标文件存在，先删除它
    if (fs::exists(originalExePath)) {
        print("目标文件已存在，准备删除...");
        if (!fs::remove(originalExePath)) {
            DWORD errorCode = GetLastError();
            print("  删除目标文件失败! 错误代码: ", errorCode, ", 错误信息: 文件删除操作失败。");
            return false;
        }
        print("  目标文件已成功删除。");
    }

    // 4. 将自身 (begeerte_loader_1.exe) 重命名为 begeerte_loader.exe (完成替换)
    if (!MoveFileA(currentExePath.string().c_str(), originalExePath.string().c_str())) {
        DWORD errorCode = GetLastError();
        print("  重命名自身程序 (替换) 失败! 错误代码: ", errorCode, ", 错误信息: 文件重命名操作失败。");
        print("  自身替换失败。");
        print("  !!!  警告：由于是无备份模式，程序可能处于不一致状态，请手动检查!"); //  !!!  重要警告 !!!
        return false;
    }
    print("  自身程序重命名 (替换) 成功: ", originalExeName);

    // 5. 启动重命名后的 begeerte_loader.exe (启动新版本)
    print("  准备启动新版本程序: ", originalExeName);
    ShellExecuteA(NULL, "open", originalExeName.c_str(), NULL, NULL, SW_SHOW);
    print("  已启动新版本程序。");

    // 6.  子进程执行完毕，退出
    print("  替换子进程执行完毕，即将退出...");
    return true; // 返回 true 表示替换成功
}



/*
 * launchRenameProcess()
 * -----------------------------
 *  启动自身 (begeerte_loader_1.exe) 的另一个进程，并传递 "__RENAME_PROCESS__" 命令行参数
 *  用于在独立的子进程中执行延迟重命名和启动操作
 */
bool launchRenameProcess() {
    char currentExePathBuffer[MAX_PATH] = { 0 };
    if (!GetModuleFileNameA(nullptr, currentExePathBuffer, MAX_PATH)) {
        print("获取自身程序路径失败，无法启动替换子进程。");
        return false;
    }
    std::string currentExePath = currentExePathBuffer;

    print("准备启动替换子进程...");
    SHELLEXECUTEINFOA sei = { sizeof(SHELLEXECUTEINFOA) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = NULL;
    sei.lpVerb = "open";
    sei.lpFile = currentExePath.c_str(); //  !!!  启动自身 (begeerte_loader_1.exe)  !!!
    sei.lpParameters = "__RENAME_PROCESS__"; //  !!!  传递 "__RENAME_PROCESS__" 命令行参数 !!!
    sei.lpDirectory = fs::current_path().string().c_str();
    sei.nShow = SW_HIDE; //  !!!  隐藏子进程窗口 !!!
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
 * 1. 设置控制台标题，并提示用户不要重命名 DLL 文件；
 * 2.  !!!  在程序头部检查是否需要执行自身替换 (更新流程)  !!!
 * 3. 自动更新检查；
 * 4. 随机重命名自身 EXE 文件（防止被检测）；
 * 5. 让用户选择游戏，并根据映射关系确定目标进程名和 DLL 名称；
 * 6. 检查黑名单进程，等待目标进程启动（满足注入条件后延时 EXTRA_DELAY_SECONDS 秒）；
 * 7. 下载 DLL 文件（仅输出下载过程提示信息），并随机重命名下载的 DLL 文件；
 * 8. 执行 DLL 注入；
 * 9. 注入任务结束后退出控制台窗口，并每秒尝试删除 DLL 文件，直至成功删除后结束进程。
 */
int mainEntry(int argc, char* argv[]) {
    //  !!! 声明 currentExePathBuffer 变量 !!!
    char currentExePathBuffer[MAX_PATH] = { 0 };

    //  !!!  1. 在程序入口处检查命令行参数，判断是否为替换子进程  !!!
    bool isRenameProcess = false;
    for (int i = 1; i < argc; ++i) { // 遍历命令行参数 (跳过 argv[0]，argv[0] 是程序自身路径)
        if (std::string(argv[i]) == "__RENAME_PROCESS__") {
            isRenameProcess = true;
            break;
        }
    }

    if (isRenameProcess) {
        //  !!!  当前进程是以 "__RENAME_PROCESS__" 参数启动的，执行延迟重命名和启动逻辑 (子进程)  !!!
        print("程序以替换子进程模式启动，准备执行延迟重命名和启动...");
        executeDelayedRenameAndLaunch(); // 调用新的延迟重命名和启动函数
        return 0; // 子进程完成替换后退出
    }


    //  !!!  2.  (如果不是替换子进程)  检查是否需要执行自身替换 (更新流程 - 父进程逻辑)  !!!
    GetModuleFileNameA(nullptr, currentExePathBuffer, MAX_PATH);
    std::string currentExePath = currentExePathBuffer;
    fs::path exeFileName = fs::path(currentExePath).filename();
    std::string tempExeNameHint = "begeerte_loader_1.exe"; //  !!!  临时 EXE 文件名提示 (请根据您的实际设置修改) !!!
    if (exeFileName.string() == tempExeNameHint) { //  !!!  检查自身是否是以临时文件名启动 !!!
        print("程序以临时版本 ", tempExeNameHint, " 启动，准备启动替换子进程...");
        if (launchRenameProcess()) { //  !!!  调用新的 launchRenameProcess 函数启动子进程  !!!
            print("替换子进程已启动，父进程即将退出。");
            return 0; // 父进程启动子进程后退出
        }
        else {
            print("启动替换子进程失败，但将继续尝试以临时版本运行 (可能功能不完整).");
            //  !!!  启动子进程失败，可以选择退出程序，或者继续以临时版本运行  !!!
            //  !!!  这里选择继续运行，但需要注意可能功能不完整  !!!
            //  !!!  您可以根据您的需求修改这里的逻辑，例如直接 shutdown() 退出 !!!
        }
    }

    // 设置控制台标题（例如项目链接）
    SetConsoleTitleW(L"github.com/zetsr/begeerte");
    print("请勿重命名DLL文件！！！");

    // 远程服务器 URL，用于下载 update.log 和 begeerte_loader.exe
    const std::string remoteServerUrl = "https://zetsr.github.io/begeerte_/";

    // 自动更新检查
    std::string updateLogUrl = remoteServerUrl + "update.log";
    std::string newExeUrl = remoteServerUrl + "begeerte_loader.exe";
    std::string localNewExePath = fs::current_path().string() + "\\begeerte_loader_1.exe"; //  !!!  下载的新 exe 保存为 begeerte_loader_1.exe !!!

    print("检查更新...");
    std::string serverVersion = readLogContent(updateLogUrl);

    if (!serverVersion.empty() && serverVersion != currentVersion) {
        print("发现新版本: ", serverVersion, ", 当前版本: ", currentVersion);
        print("准备下载更新...");
        if (DownloadFile(newExeUrl, localNewExePath)) {
            print("更新下载完成，准备启动新版本...");

            // 启动新下载的临时程序 (begeerte_loader_1.exe)，然后退出当前程序
            ShellExecuteA(NULL, "open", localNewExePath.c_str(), NULL, NULL, SW_SHOW);
            print("正在启动新版本 (临时版本), 旧版本即将退出...");
            return 0; // 退出当前程序 (旧版本 exe)
        }
        else {
            print("下载更新失败，将继续运行当前版本。");
        }
    }
    else {
        print("当前已是最新版本，无需更新。");
    }


    // 重命名自身 EXE 文件（防止检测）：生成随机文件名，并将当前 EXE 移动为新名称
    char currentExePath_rename[MAX_PATH] = { 0 };
    GetModuleFileNameA(nullptr, currentExePath_rename, MAX_PATH);
    std::string currentExe_rename(currentExePath_rename);
    std::string newExeName = generateRandomFileName(10) + ".exe";
    std::string newExePath = fs::current_path().string() + "\\" + newExeName;
    if (!MoveFileA(currentExe_rename.c_str(), newExePath.c_str())) {
        print("重命名程序失败。");
        return 1;
    }

    // 定义游戏与对应目标进程名称及 DLL 名称的映射关系
    std::map<std::string, std::pair<std::string, std::string>> gameToProcessAndDll = {
        { "Counter-Strike 2",         { "cs2.exe",                     "begeerte_cs2"         } },
        { "Day of Dragons",             { "Dragons-Win64-Shipping.exe","begeerte_dod"         } },
        { "Day of Dragons [Test]",     { "Dragons-Win64-Test.exe",    "begeerte_dod_test" } },
        { "Draconia",                 { "Draconia-Win64-Shipping.exe", "begeerte_draconia" } },
        { "Dragons Legacy",             { "Dragons-Win64-Shipping.exe",     "begeerte_dl"      } },
        { "Golden Treasure: The Great Green", { "Golden Treasure - The Great Green.exe", "begeerte_gttgg" } }
    };

    // 定义每个游戏对应的黑名单进程名称（如果存在）
    std::map<std::string, std::vector<std::string>> blacklistedProcesses = {
        { "Counter-Strike 2",         { } },
        { "Day of Dragons",             { "EasyAntiCheat_EOS.exe" } },
        { "Day of Dragons [Test]",     { "EasyAntiCheat_EOS.exe" } },
        { "Draconia",                 { } },
        { "Dragons Legacy",             { } },
        { "Golden Treasure: The Great Green", { } }
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

    // 延时等待，确保目标进程满足 DLL 注入条件
    std::this_thread::sleep_for(std::chrono::seconds(EXTRA_DELAY_SECONDS));

    // 开始下载 DLL 文件
    // 构造下载 URL（仅用于构造，不在输出中显示地址）和本地保存路径
    const std::string remoteServerUrl_dll = "https://zetsr.github.io/begeerte_/";
    std::string remoteDllUrl = remoteServerUrl_dll + targetDllName + ".dll";
    std::string localDllPath = fs::current_path().string() + "\\" + targetDllName + ".dll";

    if (!DownloadFile(remoteDllUrl, localDllPath)) {
        print("未能成功下载 DLL 文件。");
        shutdown();
    }

    // 下载成功后，使用随机文件名重命名 DLL 文件，更新全局变量 g_dllFilePath 用于后续删除
    std::string randomDllName = generateRandomFileName(10) + ".dll";
    std::string randomDllPath = fs::current_path().string() + "\\" + randomDllName;
    if (!MoveFileA(localDllPath.c_str(), randomDllPath.c_str())) {
        // 若重命名失败，则记录原下载文件路径供清理时尝试删除
        g_dllFilePath = localDllPath;
        print("重命名 DLL 文件失败。");
        shutdown();
    }
    // 更新全局变量，记录重命名后的 DLL 路径
    g_dllFilePath = randomDllPath;
    print("DLL 文件已重命名为: ", randomDllName);

    // 执行 DLL 注入操作
    if (InjectDll(targetProcessID, randomDllPath)) {
        print("DLL 注入成功！");
    }
    else {
        print("DLL 注入失败。");
    }

    // 注入任务结束后，调用 shutdown() 退出控制台并进入删除 DLL 文件循环，
    // 直至成功删除后彻底结束进程
    shutdown();
    return 0;
}


//  为了兼容性，main 函数需要调用 mainEntry() 函数
int main(int argc, char* argv[]) {
    return mainEntry(argc, argv);
}