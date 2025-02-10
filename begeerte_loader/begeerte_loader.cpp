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
#include <Print.h>
#pragma comment(lib, "urlmon.lib")  // 链接 urlmon.lib

namespace fs = std::filesystem;

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
    LPVOID loadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA");
    if (!loadLibraryAddr) {
        print("无法获取 LoadLibraryA 的地址。");
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

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
 * DownloadDll()
 * -------------
 * 利用 URLDownloadToFileA 从远程服务器下载 DLL 文件到本地指定路径。
 * 注意：仅输出下载过程提示信息，不显示远程服务器地址或 IP 信息。
 */
bool DownloadDll(const std::string& remoteUrl, const std::string& localPath) {
    print("开始下载 DLL...");
    HRESULT hr = URLDownloadToFileA(nullptr, remoteUrl.c_str(), localPath.c_str(), 0, nullptr);
    if (hr == S_OK) {
        print("DLL 下载成功。");
        return true;
    }
    else {
        print("下载 DLL 失败，HRESULT：", hr);
        return false;
    }
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
        int elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime).count());
        int remaining = timeoutSeconds - elapsed;
        if (remaining <= 0) {
            print("超时！在 ", timeoutSeconds, " 秒内未找到目标进程。");
            shutdown();
        }
        print("等待游戏启动，剩余时间: ", remaining, " 秒...");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

/*
 * main()
 * ------
 * 程序主入口：
 * 1. 设置控制台标题，并提示用户不要重命名 DLL 文件；
 * 2. 随机重命名自身 EXE 文件（防止被检测）；
 * 3. 让用户选择游戏，并根据映射关系确定目标进程名和 DLL 名称；
 * 4. 检查黑名单进程，等待目标进程启动（满足注入条件后延时 EXTRA_DELAY_SECONDS 秒）；
 * 5. 下载 DLL 文件（仅输出下载过程提示信息），并随机重命名下载的 DLL 文件；
 * 6. 执行 DLL 注入；
 * 7. 注入任务结束后退出控制台窗口，并每秒尝试删除下载的 DLL 文件，直至删除成功后结束进程。
 */
int main() {
    // 设置控制台标题（例如项目链接）
    SetConsoleTitle(L"github.com/zetsr/begeerte");
    print("请勿重命名DLL文件！！！");

    // 重命名自身 EXE 文件（防止检测）：生成随机文件名，并将当前 EXE 移动为新名称
    char currentExePath[MAX_PATH] = { 0 };
    GetModuleFileNameA(nullptr, currentExePath, MAX_PATH);
    std::string currentExe(currentExePath);
    std::string newExeName = generateRandomFileName(10) + ".exe";
    std::string newExePath = fs::current_path().string() + "\\" + newExeName;
    if (!MoveFileA(currentExe.c_str(), newExePath.c_str())) {
        print("重命名程序失败。");
        return 1;
    }

    // 定义游戏与对应目标进程名称及 DLL 名称的映射关系
    std::map<std::string, std::pair<std::string, std::string>> gameToProcessAndDll = {
        { "Counter-Strike 2",            { "cs2.exe",                      "begeerte_cs2"   } },
        { "Day of Dragons",              { "Dragons-Win64-Shipping.exe",   "begeerte_dod"   } },
        { "Day of Dragons [Test]",       { "Dragons-Win64-Test.exe",       "begeerte_dod_test" } },
        { "Draconia",                    { "Draconia-Win64-Shipping.exe",  "begeerte_draconia" } },
        { "Dragons Legacy",              { "Dragons-Win64-Shipping.exe",   "begeerte_dl"    } },
        { "Golden Treasure: The Great Green", { "Golden Treasure - The Great Green.exe", "begeerte_gttgg" } }
    };

    // 定义每个游戏对应的黑名单进程名称（如果存在）
    std::map<std::string, std::vector<std::string>> blacklistedProcesses = {
        { "Counter-Strike 2",            { } },
        { "Day of Dragons",              { "EasyAntiCheat_EOS.exe" } },
        { "Day of Dragons [Test]",       { "EasyAntiCheat_EOS.exe" } },
        { "Draconia",                    { } },
        { "Dragons Legacy",              { } },
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
    const std::string remoteServerUrl = "https://zetsr.github.io/begeerte_/"; // 仅用于构造 URL
    std::string remoteDllUrl = remoteServerUrl + targetDllName + ".dll";
    std::string localDllPath = fs::current_path().string() + "\\" + targetDllName + ".dll";

    if (!DownloadDll(remoteDllUrl, localDllPath)) {
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
