#include "pch.h" 
#include <thread>
#include <atomic>
#include "Utils.h"
#include "HookMemory.h"
#include "RunHook.h"

// 声明并定义全局的原子布尔值，用于控制线程的运行状态
std::atomic<bool> running(false); // 线程运行标志

// 用于分离打印相关的功能
void SetupConsole() {
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CONOUT$", "w", stdout);
}

// 用于分离清理控制台的功能
void CleanupConsole() {
    FreeConsole();
}

// 线程处理函数（用于锁定内存值的函数）
void lockMemoryValue() {
    while (running) {
        modifyMemoryValue();
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 也许应该钩住渲染器并在渲染函数中调用
    }
}

void OnProcessAttach() {
    SetupConsole();
    print("DLL_PROCESS_ATTACH called.");
    printProcessInfo();
    
    running = true;  // 设置线程运行标志
    std::thread(lockMemoryValue).detach();  // 启动线程并分离
}

// 退出函数
void OnShutDown() {
    print("DLL_PROCESS_SHUT_DOWN called.");
    running = false;  // 清除线程运行标志

    // 确保线程结束
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    CleanupConsole();
}



BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        OnProcessAttach();
        break;
    case DLL_PROCESS_DETACH:
        OnShutDown();
        break;
    }
    return TRUE;
}