#include "Console.h"
#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <Windows.h>
#include <vector>

// ȫ�ֱ���
std::atomic<bool> running(false);
std::vector<ConsoleEventListener> eventListeners;  // �洢����ע��ļ�����
int inputId = 0;  // ��������Ψһ���¼� ID

// ���ÿ���̨
void SetupConsole() {
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    SetConsoleTitle(L"github.com/zetsr/begeerte");

    // ���ñ�׼��������stdin���������ȡ����̨����
    freopen_s(&stream, "CONIN$", "r", stdin);
}

// �������̨
void CleanupConsole() {
    FreeConsole();
}

// ��������̨�������
void StartConsoleListening() {
    running = true;
    std::thread([]() {
        std::string userInput;
        while (running) {
            if (std::getline(std::cin, userInput)) {
                if (!userInput.empty()) {
                    // �����¼�
                    ConsoleEvent event = { inputId++, userInput };

                    // ��������ע��ļ�����
                    for (const auto& listener : eventListeners) {
                        listener(event);
                    }
                }
            }
        }
        }).detach();
}

// ֹͣ����̨�������
void StopConsoleListening() {
    running = false;
}

// ע�������¼�������
void RegisterConsoleEventListener(ConsoleEventListener listener) {
    eventListeners.push_back(listener);
}
