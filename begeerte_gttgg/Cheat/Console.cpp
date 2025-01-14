#include "Console.h"
#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <Windows.h>
#include <vector>

// 全局变量
std::atomic<bool> running(false);
std::vector<ConsoleEventListener> eventListeners;  // 存储所有注册的监听器
int inputId = 0;  // 用于生成唯一的事件 ID

// 设置控制台
void SetupConsole() {
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    SetConsoleTitle(L"github.com/zetsr/begeerte");

    // 设置标准输入流（stdin），允许读取控制台输入
    freopen_s(&stream, "CONIN$", "r", stdin);
}

// 清理控制台
void CleanupConsole() {
    FreeConsole();
}

// 启动控制台输入监听
void StartConsoleListening() {
    running = true;
    std::thread([]() {
        std::string userInput;
        while (running) {
            if (std::getline(std::cin, userInput)) {
                if (!userInput.empty()) {
                    // 生成事件
                    ConsoleEvent event = { inputId++, userInput };

                    // 触发所有注册的监听器
                    for (const auto& listener : eventListeners) {
                        listener(event);
                    }
                }
            }
        }
        }).detach();
}

// 停止控制台输入监听
void StopConsoleListening() {
    running = false;
}

// 注册输入事件监听器
void RegisterConsoleEventListener(ConsoleEventListener listener) {
    eventListeners.push_back(listener);
}
