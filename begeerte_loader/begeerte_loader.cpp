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
#include <Print.h>

namespace fs = std::filesystem;

int max_timed_out_time = 300;
int max_delay_inject_time = 3;

void shutdown() {
    // print("任务结束，倒计时三秒后退出");
    for (int i = 3; i > 0; --i) {
        // print(i, "...");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    exit(0);
}

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

std::vector<std::string> ListDllFiles(const std::string& dir) {
    std::vector<std::string> dllFiles;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".dll") {
            dllFiles.push_back(entry.path().string());
        }
    }
    return dllFiles;
}

std::vector<std::pair<DWORD, std::string>> ListProcesses() {
    std::vector<std::pair<DWORD, std::string>> processes;
    PROCESSENTRY32 pe32;
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return processes;
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            std::wstring wideString(pe32.szExeFile);
            std::string exeFile(wideString.begin(), wideString.end());
            processes.push_back({ pe32.th32ProcessID, exeFile });
        } while (Process32Next(hProcessSnap, &pe32));
    }
    CloseHandle(hProcessSnap);
    return processes;
}

bool InjectDll(DWORD processID, const std::string& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProcess == NULL) {
        print("无法打开目标进程。");
        return false;
    }
    LPVOID allocatedMemory = VirtualAllocEx(hProcess, NULL, dllPath.size() + 1, MEM_COMMIT, PAGE_READWRITE);
    if (allocatedMemory == NULL) {
        print("无法在目标进程中分配内存。");
        CloseHandle(hProcess);
        return false;
    }
    if (!WriteProcessMemory(hProcess, allocatedMemory, dllPath.c_str(), dllPath.size() + 1, NULL)) {
        print("无法将 DLL 路径写入目标进程。");
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    LPVOID loadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA");
    if (loadLibraryAddr == NULL) {
        print("无法获取 LoadLibraryA 的地址。");
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, allocatedMemory, 0, NULL);
    if (hThread == NULL) {
        print("无法在目标进程中创建远程线程。");
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    return true;
}

void checkBlacklistedProcesses(const std::map<std::string, std::vector<std::string>>& blacklistedProcesses, const std::string& selectedGame) {
    if (blacklistedProcesses.find(selectedGame) == blacklistedProcesses.end()) {
        return; // No blacklisted processes for this game.
    }

    auto processes = ListProcesses();
    for (const auto& process : processes) {
        for (const auto& blacklistedProcess : blacklistedProcesses.at(selectedGame)) {
            if (process.second == blacklistedProcess) {
                print("警告：检测到黑名单进程 (", blacklistedProcess, ")");
                shutdown();
            }
        }
    }
}

int main() {
    SetConsoleTitle(L"github.com/zetsr/begeerte");

    print("请勿重命名DLL文件！！！");

    char currentExePath[MAX_PATH];
    GetModuleFileNameA(NULL, currentExePath, MAX_PATH);
    std::string currentExe(currentExePath);
    std::string newExeName = generateRandomFileName(10) + ".exe";
    std::string newExePath = fs::current_path().string() + "\\" + newExeName;
    if (!MoveFileA(currentExe.c_str(), newExePath.c_str())) {
        print("重命名程序失败。");
        return 1;
    }

    std::map<std::string, std::pair<std::string, std::string>> gameToProcessAndDll = {
        { "Day of Dragons", { "Dragons-Win64-Shipping.exe", "begeerte_dod" } },
        { "Golden Treasure: The Great Green", { "Golden Treasure - The Great Green.exe", "begeerte_gttgg" } }
    };

    std::map<std::string, std::vector<std::string>> blacklistedProcesses = {
        { "Day of Dragons", { "EasyAntiCheat_EOS.exe" } },
        { "Golden Treasure: The Great Green", {} }
    };

    std::cout << "请选择游戏:\n";
    for (size_t i = 0; i < gameToProcessAndDll.size(); ++i) {
        auto it = gameToProcessAndDll.begin();
        std::advance(it, i);
        print(i + 1, ". " , it->first);
    }

    int gameChoice;
    std::cin >> gameChoice;
    if (gameChoice < 1 || gameChoice > static_cast<int>(gameToProcessAndDll.size())) {
        print("无效的游戏选择。");
        return 1;
    }

    auto selectedGame = gameToProcessAndDll.begin();
    std::advance(selectedGame, gameChoice - 1);
    std::string targetProcessName = selectedGame->second.first;
    std::string targetDllName = selectedGame->second.second;

    checkBlacklistedProcesses(blacklistedProcesses, selectedGame->first);

    std::vector<std::string> dllFiles = ListDllFiles(fs::current_path().string());
    std::string targetDllPath;
    for (const auto& dllFile : dllFiles) {
        if (fs::path(dllFile).filename().string() == targetDllName + ".dll") {
            targetDllPath = dllFile;
            break;
        }
    }

    if (targetDllPath.empty()) {
        print("未找到对应的 DLL 文件。");
        shutdown();
    }

    DWORD targetProcessID = 0;
    auto startTime = std::chrono::steady_clock::now();
    while (true) {
        std::vector<std::pair<DWORD, std::string>> processes = ListProcesses();
        for (const auto& process : processes) {
            if (process.second == targetProcessName) {
                targetProcessID = process.first;
                break;
            }
        }

        if (targetProcessID) {
            print("找到目标进程 ", targetProcessName, "，进程 ID: ", targetProcessID);
            break;
        }

        int remainingTime = max_timed_out_time - std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime).count();
        if (remainingTime <= 0) {
            print("超时！在 ", max_timed_out_time, " 秒内未找到目标进程。");
            shutdown();
        }
        print("等待游戏启动，剩余时间: ", remainingTime, " 秒...");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::this_thread::sleep_for(std::chrono::seconds(max_delay_inject_time));
    if (InjectDll(targetProcessID, targetDllPath)) {
        // print("DLL 注入成功！");
    }
    else {
        print("DLL 注入失败。");
        shutdown();
    }

    shutdown();
    return 0;
}
