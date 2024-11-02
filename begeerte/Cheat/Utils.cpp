#include "..\pch.h"
#include "Utils.h"
#include <windows.h>
#include <iostream>
#include <Psapi.h>

// ���� running ��������Ҫ������
extern std::atomic<bool> running;

// ����һ����Ϊ print �ĺ����������ڿ���̨�����Ϣ
extern "C" __declspec(dllexport) void print(const char* message) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    std::cout << message << std::endl;
}

// �� TCHAR ת��Ϊ std::string
std::string tcharToString(const TCHAR* tcharStr) {
    if (tcharStr == nullptr) {
        return ""; // �������Ϊ nullptr�����ؿ��ַ���
    }

    int size = WideCharToMultiByte(CP_UTF8, 0, tcharStr, -1, NULL, 0, NULL, NULL);
    if (size <= 0) {
        return ""; // ���ת��ʧ�ܣ����ؿ��ַ���
    }

    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, tcharStr, -1, &result[0], size, NULL, NULL);
    return result;
}

// ��ӡ��ǰ���̺ʹ��ڵ���Ϣ
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
