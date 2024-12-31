#include <iostream>  
#include <windows.h>
#include <vector>
#include <string>
#include <filesystem>
#include <tlhelp32.h>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;
int max_timed_out_time = 300;
int max_delay_inject_time = 3;
int max_shutdown_time = 5;

// 函数：列出当前目录下的 DLL 文件
std::vector<std::string> ListDllFiles(const std::string& dir) {
    std::vector<std::string> dllFiles;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".dll") {
            dllFiles.push_back(entry.path().string());
        }
    }
    return dllFiles;
}

// 函数：列出所有正在运行的进程
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
            // 使用 std::wstring_convert 将宽字符数组转换为 std::string
            std::wstring wideString(pe32.szExeFile);
            std::string exeFile(wideString.begin(), wideString.end());  // 转换为 std::string
            processes.push_back({ pe32.th32ProcessID, exeFile });
        } while (Process32Next(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
    return processes;
}

// 函数：注入 DLL 到目标进程
bool InjectDll(DWORD processID, const std::string& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProcess == NULL) {
        std::cerr << "Failed to open target process.\n";
        return false;
    }

    // 获取 DLL 的路径地址
    LPVOID allocatedMemory = VirtualAllocEx(hProcess, NULL, dllPath.size() + 1, MEM_COMMIT, PAGE_READWRITE);
    if (allocatedMemory == NULL) {
        std::cerr << "Failed to allocate memory in target process.\n";
        CloseHandle(hProcess);
        return false;
    }

    // 写入 DLL 路径
    if (!WriteProcessMemory(hProcess, allocatedMemory, dllPath.c_str(), dllPath.size() + 1, NULL)) {
        std::cerr << "Failed to write DLL path into target process.\n";
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 创建远程线程来加载 DLL
    LPVOID loadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA");
    if (loadLibraryAddr == NULL) {
        std::cerr << "Failed to get address of LoadLibraryA.\n";
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, allocatedMemory, 0, NULL);
    if (hThread == NULL) {
        std::cerr << "Failed to create remote thread in target process.\n";
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);

    // 清理
    VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    return true;
}

int main() {
    // 获取当前目录下的所有 DLL 文件
    std::string currentDir = fs::current_path().string();
    std::vector<std::string> dllFiles = ListDllFiles(currentDir);

    for (size_t i = 0; i < dllFiles.size(); ++i) {
        std::cout << i + 1 << ". " << fs::path(dllFiles[i]).filename().string() << "\n";
    }

    if (dllFiles.empty()) {
        std::cerr << "No DLL files found in the current directory.\n";
        return 1;
    }

    std::cout << "Enter the number of the DLL to inject: ";
    int dllChoice;
    std::cin >> dllChoice;
    std::cin.ignore();  // 清除换行符

    if (dllChoice < 1 || dllChoice > dllFiles.size()) {
        std::cerr << "Invalid DLL choice.\n";
        return 1;
    }

    std::string targetDll = dllFiles[dllChoice - 1];

    // 查找目标进程 "Dragons-Win64-Shipping.exe"
    bool processFound = false;
    DWORD targetProcessID = 0;
    const std::string targetProcessName = "Dragons-Win64-Shipping.exe";
    auto startTime = std::chrono::steady_clock::now();

    // 打印进程未找到的警告并开始倒计时
    std::cout << "Game didn't start, maximum waiting time is " << max_timed_out_time << " seconds.\n";

    while (true) {
        std::vector<std::pair<DWORD, std::string>> processes = ListProcesses();
        for (const auto& process : processes) {
            if (process.second == targetProcessName) {
                targetProcessID = process.first;
                processFound = true;
                break;
            }
        }

        if (processFound) {
            std::cout << "Found target process " << targetProcessName << " with PID: " << targetProcessID << "\n";
            break;
        }

        // 检查是否超过最大等待时间300秒
        auto elapsedTime = std::chrono::steady_clock::now() - startTime;
        int remainingTime = max_timed_out_time - std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count();

        if (remainingTime <= 0) {
            std::cerr << "Timeout! Could not find target process within " << max_timed_out_time << " seconds.\n";
            return 1;
        }

        // 输出剩余的倒计时
        std::cout << "Time remaining: " << remainingTime << " seconds...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));  // 每秒钟检查一次
    }

    // 如果找到了进程，等待3秒后执行注入
    if (processFound) {
        std::cout << "Waiting for " << max_delay_inject_time << " seconds before injecting DLL...\n";
        std::this_thread::sleep_for(std::chrono::seconds(max_delay_inject_time));  // 等待3秒

        // 执行 DLL 注入
        if (InjectDll(targetProcessID, targetDll)) {
            std::cout << "DLL injected successfully!\n";
        }
        else {
            std::cerr << "DLL injection failed.\n";
        }
    }

    // 等待5秒钟后自动退出
    for (int i = max_shutdown_time; i > 0; --i) {
        std::cout << "Exiting in " << i << " seconds...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
