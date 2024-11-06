#include "..\pch.h" 
#include "HookMemory.h"
#include "Utils.h" // 引入 Utils.h 以使用 print 函数

// HookMemory 的构造函数
HookMemory::HookMemory(HANDLE process, uintptr_t base, uintptr_t firstOffset, uintptr_t finalOffset)
    : hProcess(process), moduleBaseAddress(base), firstOffset(firstOffset), finalOffset(finalOffset) {}

// 获取当前值的函数
int HookMemory::GetInt() {
    uintptr_t address = moduleBaseAddress + firstOffset; // 基地址 + 偏移
    uintptr_t finalAddress;

    // 读取内存地址
    if (!ReadProcessMemory(hProcess, (BYTE*)address, &finalAddress, sizeof(finalAddress), NULL)) {
        print(("Failed to read memory for Get() at address: " + std::to_string(address) + ". Returning 0.").c_str());
        return 0;
    }

    finalAddress += finalOffset; // 添加最终偏移
   int currentValue;
    if (ReadProcessMemory(hProcess, (BYTE*)finalAddress, &currentValue, sizeof(currentValue), NULL)) {
        /*print(("Successfully read current value: " + std::to_string(currentValue) + " from address: " + std::to_string(finalAddress) +
            " (base: " + std::to_string(moduleBaseAddress) + ", first offset: " + std::to_string(firstOffset) + ", final offset: " + std::to_string(finalOffset) + ").").c_str());
        */
        return currentValue;
    }
    else {
        print(("Failed to read current memory for Get() at address: " + std::to_string(finalAddress) + ". Returning 0.").c_str());
        return 0;
    }
}

// 获取当前值的函数
float HookMemory::GetFloat() {
    uintptr_t address = moduleBaseAddress + firstOffset; // 基地址 + 帧偏移
    uintptr_t finalAddress;

    // 读取内存地址
    if (!ReadProcessMemory(hProcess, (BYTE*)address, &finalAddress, sizeof(finalAddress), NULL)) {
        print(("Failed to read memory for Get() at address: " + std::to_string(address) + ". Returning 0.").c_str());
        return 0;
    }

    finalAddress += finalOffset; // 添加最终偏移
    float currentValue;
    if (ReadProcessMemory(hProcess, (BYTE*)finalAddress, &currentValue, sizeof(currentValue), NULL)) {
        /*print(("Successfully read current value: " + std::to_string(currentValue) + " from address: " + std::to_string(finalAddress) +
            " (base: " + std::to_string(moduleBaseAddress) + ", frame offset: " + std::to_string(frameGenOffset) + ", final offset: " + std::to_string(finalOffset) + ").").c_str());
        */
        return currentValue;
    }
    else {
        print(("Failed to read current memory for Get() at address: " + std::to_string(finalAddress) + ". Returning 0.").c_str());
        return 0;
    }
}

// 设置新值的函数
void HookMemory::SetInt(int value) {
    uintptr_t address = moduleBaseAddress + firstOffset; // 基地址 + 帧偏移
    uintptr_t finalAddress;

    // 读取内存地址
    if (!ReadProcessMemory(hProcess, (BYTE*)address, &finalAddress, sizeof(finalAddress), NULL)) {
        print(("Failed to read memory for Set() at address: " + std::to_string(address) + ".").c_str());
        return;
    }

    finalAddress += finalOffset; // 添加最终偏移
    // 写入新值
    if (!WriteProcessMemory(hProcess, (BYTE*)finalAddress, &value, sizeof(value), NULL)) {
        print(("Failed to write memory at address: " + std::to_string(finalAddress) +
            " (base: " + std::to_string(moduleBaseAddress) + ", first offset: " + std::to_string(firstOffset) + ", final offset: " + std::to_string(finalOffset) + ").").c_str());
    }
    else {
        print(("Memory modified to: " + std::to_string(value) + " at address: " + std::to_string(finalAddress) +
            " (base: " + std::to_string(moduleBaseAddress) + ", first offset: " + std::to_string(firstOffset) + ", final offset: " + std::to_string(finalOffset) + ").").c_str());
    }
}

// 设置新值的函数
void HookMemory::SetFloat(float value) {
    uintptr_t address = moduleBaseAddress + firstOffset; // 基地址 + 帧偏移
    uintptr_t finalAddress;

    // 读取内存地址
    if (!ReadProcessMemory(hProcess, (BYTE*)address, &finalAddress, sizeof(finalAddress), NULL)) {
        print(("Failed to read memory for Set() at address: " + std::to_string(address) + ".").c_str());
        return;
    }

    finalAddress += finalOffset; // 添加最终偏移
    // 写入新值
    if (!WriteProcessMemory(hProcess, (BYTE*)finalAddress, &value, sizeof(value), NULL)) {
        print(("Failed to write memory at address: " + std::to_string(finalAddress) +
            " (base: " + std::to_string(moduleBaseAddress) + ", first offset: " + std::to_string(firstOffset) + ", final offset: " + std::to_string(finalOffset) + ").").c_str());
    }
    else {
        print(("Memory modified to: " + std::to_string(value) + " at address: " + std::to_string(finalAddress) +
            " (base: " + std::to_string(moduleBaseAddress) + ", first offset: " + std::to_string(firstOffset) + ", final offset: " + std::to_string(finalOffset) + ").").c_str());
    }
}
