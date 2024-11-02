#include "..\pch.h"
#include "Utils.h"
#include <windows.h>
#include <iostream>
#include <Psapi.h>

// 声明 running 变量，不要定义它
extern std::atomic<bool> running;

// 导出一个名为 print 的函数，用于在控制台输出消息
extern "C" __declspec(dllexport) void print(const char* message) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    std::cout << message << std::endl;
}

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
            print(("Current Process ID: " + std::to_string(processID)).c_str());
            print(("Current Process Path: " + tcharToString(processName)).c_str());
        }
        CloseHandle(hProcess);
    }
}
