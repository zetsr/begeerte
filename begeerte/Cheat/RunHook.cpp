#include "..\pch.h" 
#include "RunHook.h"
#include "Utils.h"
#include "HookMemory.h" 
#include <string>    

// 修改 FrameGen 的函数
void HookFrameGen(HANDLE hProcess, uintptr_t moduleBaseAddress) {
    /*
    2024/11/3
    : 适配最新版本Version 1.1.3

    Version 1.1.2 实际上并未影响帧生成的内存地址和偏移。
    不知道为什么他们仍未修复这个问题，也许他们并不认为这是一个“错误”，因为除此之外我想不通其他能够对此解释的理由。
    也许应该转向sig作为扫描特征而不是使用基址+偏移的形式，因为后者几乎每次更新都需要重新逆向。
    此外，我仍然无法正确钩住d3d12，这导致几乎不可能注入imgui进行实时调试。
    
    这个项目简直糟透了...这个游戏也糟透了。
    */

    const uintptr_t firstOffset = 0x082F4980; // 0x082F1418 备用
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
    /*
    没有意义
    Quality和Ultra Quality其实区别不大
    */
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