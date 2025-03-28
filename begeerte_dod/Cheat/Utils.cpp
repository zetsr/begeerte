#include "..\pch.h"
#include "Utils.h"
#include <windows.h>
#include <iostream>
#include <Psapi.h>
#include "Print.h"

// ���� running ��������Ҫ������
extern std::atomic<bool> running;

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
            print("Current Process ID: ", processID);
            //print("Current Process Path: ", processName);
        }
        CloseHandle(hProcess);
    }
}

void printSystemInfo() {
    // ��ȡ�ڴ���Ϣ
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        // �������ڴ棨�� MB Ϊ��λ��
        DWORDLONG totalMemoryMB = statex.ullTotalPhys / (1024 * 1024);
        // ��ӡ���ڴ�
        print("Total Memory: ", totalMemoryMB, " MB");
    }
    else {
        // print("Failed to get memory status.");
    }
}