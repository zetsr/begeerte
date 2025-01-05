#pragma once
#include <string>
#include <functional>

// 事件结构体
struct ConsoleEvent {
    int id;     // 事件 ID
    std::string text;  // 输入的文本内容
};

// 监听控制台输入的回调函数类型
using ConsoleEventListener = std::function<void(const ConsoleEvent&)>;

// 控制台操作函数
void SetupConsole();
void CleanupConsole();
void StartConsoleListening();
void StopConsoleListening();

// 注册输入事件监听器
void RegisterConsoleEventListener(ConsoleEventListener listener);
