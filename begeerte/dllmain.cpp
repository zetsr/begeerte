#include "pch.h" 
#include "Utils.h"
#include "HookMemory.h"
#include <thread>
#include <atomic>

// 声明并定义全局的原子布尔值，用于控制线程的运行状态
std::atomic<bool> running(false); // 线程运行标志

// 修改 FrameGen 的函数
void modifyFrameGen(HANDLE hProcess, uintptr_t moduleBaseAddress) {
    const uintptr_t frameGenOffset = 0x07CCE080; // 帧生成的偏移
    const uintptr_t finalOffset = 0x4; // 最终偏移

    HookMemory frameGen(hProcess, moduleBaseAddress, frameGenOffset, finalOffset);

    int expectedValue = 1; // 预期值，1开启，0关闭
    int currentValue = frameGen.Get();

    if (currentValue != expectedValue) {
        frameGen.Set(expectedValue);
    }
}

// 修改内存值的函数
/*
    也许应该钩住游戏的帧阶段函数而不是使用循环以获得更低的性能开销
    在csgo中有类似的函数，frame render start/end
    idk，我不知道如何创建钩子，我不怎么懂逆向
*/
// 2024/9/30
// 目前性能良好，暂时无需改动

void modifyMemoryValue() {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
    if (hProcess == NULL) {
        print("Failed to open process.");
        return;
    }

    try {
        uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandle(L"Dragons-Win64-Shipping.exe");
        modifyFrameGen(hProcess, moduleBaseAddress); // 调用新的函数
    }
    catch (...) {
        print("An exception occurred in modifyMemoryValue.");
    }

    CloseHandle(hProcess);
}

// 线程函数，用于持续锁定内存值
void lockMemoryValue() {
    print("Thread started.");
    while (running) {
        modifyMemoryValue();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// DllMain 函数保持不变


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        AllocConsole();
        {
            FILE* stream;
            freopen_s(&stream, "CONOUT$", "w", stdout);
        }
        print("DLL_PROCESS_ATTACH called.");
        printProcessInfo();
        running = true; // 设置线程运行标志
        std::thread(lockMemoryValue).detach();
        break;
    case DLL_PROCESS_DETACH:
        print("DLL_PROCESS_DETACH called.");
        running = false; // 清除线程运行标志

        // 确保线程结束
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        FreeConsole();
        break;
    }
    return TRUE;
}
