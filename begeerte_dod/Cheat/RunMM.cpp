#include <iostream>

#include "..\pch.h" 
#include "RunMM.h"
#include "Utils.h"
#include "MM.h" 
#include "CheatData.h"
#include "Print.h"

// const void* fsr = nullptr;

/*
void m_set_int(int address, int offset, int value) {
    uintptr_t _address = (uintptr_t)GetModuleHandle(L"Dragons-Win64-Shipping.exe") + address; // 基址 + 偏移
    uintptr_t* finalAddress = (uintptr_t*)_address;  // 获取最终地址的指针

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // 通过指针调整最终地址

    int* currentValue = (int*)finalAddress;  // 获取指向目标值的指针
    *currentValue = value;
}

void m_set_float(int address, int offset, float value) {
    uintptr_t _address = (uintptr_t)GetModuleHandle(L"Dragons-Win64-Shipping.exe") + address; // 基址 + 偏移
    uintptr_t* finalAddress = (uintptr_t*)_address;  // 获取最终地址的指针

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // 通过指针调整最终地址

    float* currentValue = (float*)finalAddress;  // 获取指向目标值的指针
    *currentValue = value;
}

int m_get_int(int address, int offset) {
    uintptr_t _address = (uintptr_t)GetModuleHandle(L"Dragons-Win64-Shipping.exe") + address; // 基址 + 偏移
    uintptr_t* finalAddress = (uintptr_t*)_address;  // 获取最终地址的指针

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // 通过指针调整最终地址

    int* currentValue = (int*)finalAddress;  // 获取指向目标值的指针
    return *currentValue;
}

int m_get_float(int address, int offset) {
    uintptr_t _address = (uintptr_t)GetModuleHandle(L"Dragons-Win64-Shipping.exe") + address; // 基址 + 偏移
    uintptr_t* finalAddress = (uintptr_t*)_address;  // 获取最终地址的指针

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // 通过指针调整最终地址

    float* currentValue = (float*)finalAddress;  // 获取指向目标值的指针
    return *currentValue;
}

*/

SIZE_T GetModuleSize(uintptr_t moduleBaseAddress) {
    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T totalSize = 0;
    uintptr_t address = moduleBaseAddress;

    while (VirtualQuery((LPCVOID)address, &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT) {
            totalSize += mbi.RegionSize;
        }
        address += mbi.RegionSize;
        if (mbi.Type == MEM_IMAGE) break;  // Only interested in the image section
    }

    return totalSize;
}

SIZE_T GetPEFileSize(uintptr_t moduleBaseAddress) {
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)moduleBaseAddress;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(moduleBaseAddress + dosHeader->e_lfanew);

    // 获取文件大小
    SIZE_T sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;

    return sizeOfImage;
}

// 修改 FrameGen 的函数
void HookFrameGen(/*HANDLE hProcess, uintptr_t moduleBaseAddress*/) {

    /*
    if (!g_cheatdata->FxxkFrameGen) {
        return;
    }
    */

    /*
    * 2024/12/31
    * 彻底抛弃了RPM，感觉生活正在变得美好。
    * 指针操作确实非常酷。
    * 尝试了一下通过特征码扫描内存，不确定是否成功了。它确实能进行内存读写，但是因为没有得到正确的特征码，无法得知它是否真的正确工作。
   
    * 2024/12/16
    * 尝试创建了一个真正的钩子，它似乎可以钩住游戏的渲染函数，也许，我不确定。
    * 并且我发现创建一个Gui对我来说原比预想的更加困难。

    * 2024/11/16
    * 适配最新版本Version 1.1.7

    * 2024/11/7
    * 适配最新版本Version 1.1.4

    * 2024/11/3
    * 适配最新版本Version 1.1.3
    * Version 1.1.2 实际上并未影响帧生成的内存地址和偏移。
    * 不知道为什么他们仍未修复这个问题，也许他们并不认为这是一个“错误”，因为除此之外我想不通其他能够对此解释的理由。
    * 也许应该转向sig作为扫描特征而不是使用基址+偏移的形式，因为后者几乎每次更新都需要重新逆向。
    * 此外，我仍然无法正确钩住d3d12，这导致几乎不可能注入imgui进行实时调试。

    * 这个项目简直糟透了...这个游戏也糟透了。
    */

    /* 这些都过时了

    const uintptr_t firstOffset = 0x082E1CE0; 
    const uintptr_t finalOffset = 0x4;

    HookMemory frameGen(hProcess, moduleBaseAddress, firstOffset, finalOffset);

    int expectedValue = 1; // 1 = On，0 = Off
    int currentValue = frameGen.GetInt();

    if (currentValue != expectedValue) {
        print(("Try set FrameGen to " + std::to_string(expectedValue) +
            " at Offset: " + std::to_string(firstOffset) +
            "+" + std::to_string(finalOffset)).c_str());
        frameGen.SetInt(expectedValue);
    }
    */

    /*
    if (!fsr) {
        fsr = Sig::find((void*)g_cheatdata->moduleBaseAddress(), GetPEFileSize(g_cheatdata->moduleBaseAddress()), "48 8B C4 48 89 50 10 55 53 56 41 57");
        print("48 8B C4 48 89 50 10 55 53 56 41 57 | ", fsr);
        print("48 8B C4 48 89 50 10 55 53 56 41 57 + A0 | ", reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(fsr) + 0xA0));
        //std::cout << "[begeerte] " << std::hex << "48 8B C4 48 89 50 10 55 53 56 41 57: " << fsr << std::endl;
        //std::cout << std::hex << "48 8B C4 48 89 50 10 55 53 56 41 57 + A0: " << reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(fsr) + 0xA0) << std::endl;
    }
    */

    // int* p = reinterpret_cast<int*>(((uintptr_t) * ((void**)(fsr))) + 0xA0);

    // 另一种方法
    // 48 8B C4 48 89 50 10 55 53 56 41 57 + A0
    // int* p = reinterpret_cast<int*>(((uintptr_t) * ((void**)(moduleBaseAddress + 0x082E1CE0))) + 0x4);

    int address = 0x082E1CE0;
    int offset = 0x4;
    int value = g_cheatdata->FxxkFrameGen ? 1 : 0;

    /* 被开发者搞砸
    if (MM::GetInt(address, 0x0) == 0) {
        return; // Frame Gen UI Disabled!
    }
    */

    if (!g_cheatdata->FxxkFrameGen) {
        MM::SetInt(address, offset, value);
        return;
    }

    if (MM::GetInt(address, offset) != value) {
        MM::SetInt(address, offset, value);
        //print("Try set FrameGen to ", value, " at Offset: ", address, "+", offset);
        //std::cout << "Try set FrameGen to " << value << " at Offset: " << address << "+" << offset << std::endl;
    }
}

// 修改渲染分辨率
void HookRenderResolution(/*HANDLE hProcess, uintptr_t moduleBaseAddress*/) {

    return;

    /*
    if (!g_cheatdata->FxxkRenderResolution) {
        return;
    }
    */

    /*
    没有意义
    Quality和Ultra Quality其实区别不大
    不过可以先用着，懒得手动调
    */

    /* 这些都过时了
    const uintptr_t firstOffset = 0x085A3720;
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

    */

    int address = 0x085A3720;
    int offset = 0x0;
    float value = 75.f;

    if (MM::GetFloat(address, offset) != value) {
        MM::SetFloat(address, offset, value);
        print("Try set RenderResolution to ", value, " at Offset: ", address, "+", offset);
        //std::cout << "Try set RenderResolution to " << value << " at Offset: " << address << "+" << offset << std::endl;
    }
}

// 修改内存值的函数
void OnPresent() {

    HookFrameGen();
    HookRenderResolution();

    /*

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
    if (hProcess == NULL) {
        print("Failed to open process.");
        return;
    }

    try {
        uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandle(L"Dragons-Win64-Shipping.exe");
        HookFrameGen(moduleBaseAddress);
        HookRenderResolution(moduleBaseAddress);
    }
    catch (...) {
        print("An exception occurred by modifyMemoryValue.");
    }

    CloseHandle(hProcess);
    */
}