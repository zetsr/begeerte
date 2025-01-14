#include "MM.h"
#include "CheatData.h"
#include "Print.h"

#include <vector>
#include <stdexcept>
#include <cstdint>
#include <iostream>

// ApplyOffsets 函数，添加指针有效性检查
uintptr_t ApplyOffsets(uintptr_t address, const std::vector<int>& offsets) {
    uintptr_t currentAddress = address;
    for (int offset : offsets) {
        if (!currentAddress) {  // 检查地址是否为空
            throw std::runtime_error("Invalid memory address encountered in ApplyOffsets.");
        }
        currentAddress = *(uintptr_t*)currentAddress + offset;
    }
    return currentAddress;
}

void MM::SetInt(int address, const std::vector<int>& offsets, int value) {
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // 基址 + 偏移
    try {
        uintptr_t finalAddress = ApplyOffsets(_address, offsets);  // 通过偏移量调整最终地址
        if (!finalAddress) {  // 检查最终地址是否有效
            throw std::runtime_error("Final address is invalid in SetInt.");
        }
        int* currentValue = (int*)finalAddress;  // 获取指向目标值的指针
        *currentValue = value;
    }
    catch (const std::exception& e) {
        print("Error in SetInt: ", e.what());
    }
}

void MM::SetFloat(int address, const std::vector<int>& offsets, float value) {
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // 基址 + 偏移
    try {
        uintptr_t finalAddress = ApplyOffsets(_address, offsets);  // 通过偏移量调整最终地址
        if (!finalAddress) {  // 检查最终地址是否有效
            throw std::runtime_error("Final address is invalid in SetFloat.");
        }
        float* currentValue = (float*)finalAddress;  // 获取指向目标值的指针
        *currentValue = value;
    }
    catch (const std::exception& e) {
        print("Error in SetFloat: ", e.what());
    }
}

int MM::GetInt(int address, const std::vector<int>& offsets) {
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // 基址 + 偏移
    try {
        uintptr_t finalAddress = ApplyOffsets(_address, offsets);  // 通过偏移量调整最终地址
        if (!finalAddress) {  // 检查最终地址是否有效
            throw std::runtime_error("Final address is invalid in GetInt.");
        }
        int* currentValue = (int*)finalAddress;  // 获取指向目标值的指针
        return *currentValue;
    }
    catch (const std::exception& e) {
        print("Error in GetInt: ", e.what());
        return 0;  // 返回默认值
    }
}

float MM::GetFloat(int address, const std::vector<int>& offsets) {
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // 基址 + 偏移
    try {
        uintptr_t finalAddress = ApplyOffsets(_address, offsets);  // 通过偏移量调整最终地址
        if (!finalAddress) {  // 检查最终地址是否有效
            throw std::runtime_error("Final address is invalid in GetFloat.");
        }
        float* currentValue = (float*)finalAddress;  // 获取指向目标值的指针
        return *currentValue;
    }
    catch (const std::exception& e) {
        print("Error in GetFloat: ", e.what());
        return 0.0f;  // 返回默认值
    }
}
