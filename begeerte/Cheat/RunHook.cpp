#include "..\pch.h" 
#include "RunHook.h"
#include "Utils.h"
#include "HookMemory.h" 
#include <string>    

// 修改 FrameGen 的函数
void HookFrameGen(HANDLE hProcess, uintptr_t moduleBaseAddress) {
    const uintptr_t firstOffset = 0x082BA380;
    const uintptr_t finalOffset = 0x4;

    HookMemory frameGen(hProcess, moduleBaseAddress, firstOffset, finalOffset);

    int expectedValue = 1; // 预期值：1开启，0关闭
    int currentValue = frameGen.GetInt();

    if (currentValue != expectedValue) {
        print(("Try set FrameGen to " + std::to_string(expectedValue) +
            " at Offset: " + std::to_string(firstOffset) +
            "+" + std::to_string(finalOffset)).c_str());
        frameGen.SetInt(expectedValue);
    }
}

// 修改渲染分辨率
void HookRenderResolution(HANDLE hProcess, uintptr_t moduleBaseAddress) {
    const uintptr_t firstOffset = 0x07F7A828; // 已过时
    const uintptr_t finalOffset = 0x0;

    HookMemory RenderResolution(hProcess, moduleBaseAddress, firstOffset, finalOffset);

    float expectedValue = 75.f; // 75.f = Ultra Quality
    float currentValue = RenderResolution.GetFloat();

    if (currentValue != expectedValue) {
        print(("Try set RenderResolution to " + std::to_string(expectedValue) +
            " at Offset: " + std::to_string(firstOffset) +
            "+" + std::to_string(finalOffset)).c_str());
        RenderResolution.SetFloat(expectedValue);
    }
}

// 修改内存值的函数
void modifyMemoryValue() {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
    if (hProcess == NULL) {
        print("Failed to open process.");
        return;
    }

    try {
        uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandle(L"Dragons-Win64-Shipping.exe");
        HookFrameGen(hProcess, moduleBaseAddress);
        //HookRenderResolution(hProcess, moduleBaseAddress);
    }
    catch (...) {
        print("An exception occurred by modifyMemoryValue.");
    }

    CloseHandle(hProcess);
}