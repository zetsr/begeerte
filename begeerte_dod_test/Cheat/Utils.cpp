#include "..\pch.h"
#include "Utils.h"
#include <windows.h>
#include <iostream>
#include <Psapi.h>
#include "Print.h"

// 声明 running 变量，不要定义它
extern std::atomic<bool> running;

// 将 TCHAR 转换为 std::string
std::string tcharToString(const TCHAR* tcharStr) {
    if (tcharStr == nullptr) {
        return ""; // 如果输入为 nullptr，返回空字符串
    }

    int size = WideCharToMultiByte(CP_UTF8, 0, tcharStr, -1, NULL, 0, NULL, NULL);
    if (size <= 0) {
        return ""; // 如果转换失败，返回空字符串
    }

    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, tcharStr, -1, &result[0], size, NULL, NULL);
    return result;
}

// 打印当前进程和窗口的信息
void printProcessInfo() {
    DWORD processID = GetCurrentProcessId();
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

    if (hProcess) {
        TCHAR processName[MAX_PATH];
        if (GetModuleFileNameEx(hProcess, NULL, processName, sizeof(processName) / sizeof(TCHAR))) {
            print("Current Process ID: ", processID);
            //print("Current Process Path: ", processName);
        }
        CloseHandle(hProcess);
    }
}

void printSystemInfo() {
    // 获取内存信息
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        // 计算总内存（以 MB 为单位）
        DWORDLONG totalMemoryMB = statex.ullTotalPhys / (1024 * 1024);
        // 打印总内存
        print("Total Memory: ", totalMemoryMB, " MB");
    }
    else {
        // print("Failed to get memory status.");
    }
}