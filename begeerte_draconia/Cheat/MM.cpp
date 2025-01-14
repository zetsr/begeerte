#include "MM.h"
#include "CheatData.h"
#include "Print.h"

#include <vector>
#include <stdexcept>
#include <cstdint>
#include <iostream>

// ApplyOffsets ���������ָ����Ч�Լ��
uintptr_t ApplyOffsets(uintptr_t address, const std::vector<int>& offsets) {
    uintptr_t currentAddress = address;
    for (int offset : offsets) {
        if (!currentAddress) {  // ����ַ�Ƿ�Ϊ��
            throw std::runtime_error("Invalid memory address encountered in ApplyOffsets.");
        }
        currentAddress = *(uintptr_t*)currentAddress + offset;
    }
    return currentAddress;
}

void MM::SetInt(int address, const std::vector<int>& offsets, int value) {
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // ��ַ + ƫ��
    try {
        uintptr_t finalAddress = ApplyOffsets(_address, offsets);  // ͨ��ƫ�����������յ�ַ
        if (!finalAddress) {  // ������յ�ַ�Ƿ���Ч
            throw std::runtime_error("Final address is invalid in SetInt.");
        }
        int* currentValue = (int*)finalAddress;  // ��ȡָ��Ŀ��ֵ��ָ��
        *currentValue = value;
    }
    catch (const std::exception& e) {
        print("Error in SetInt: ", e.what());
    }
}

void MM::SetFloat(int address, const std::vector<int>& offsets, float value) {
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // ��ַ + ƫ��
    try {
        uintptr_t finalAddress = ApplyOffsets(_address, offsets);  // ͨ��ƫ�����������յ�ַ
        if (!finalAddress) {  // ������յ�ַ�Ƿ���Ч
            throw std::runtime_error("Final address is invalid in SetFloat.");
        }
        float* currentValue = (float*)finalAddress;  // ��ȡָ��Ŀ��ֵ��ָ��
        *currentValue = value;
    }
    catch (const std::exception& e) {
        print("Error in SetFloat: ", e.what());
    }
}

int MM::GetInt(int address, const std::vector<int>& offsets) {
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // ��ַ + ƫ��
    try {
        uintptr_t finalAddress = ApplyOffsets(_address, offsets);  // ͨ��ƫ�����������յ�ַ
        if (!finalAddress) {  // ������յ�ַ�Ƿ���Ч
            throw std::runtime_error("Final address is invalid in GetInt.");
        }
        int* currentValue = (int*)finalAddress;  // ��ȡָ��Ŀ��ֵ��ָ��
        return *currentValue;
    }
    catch (const std::exception& e) {
        print("Error in GetInt: ", e.what());
        return 0;  // ����Ĭ��ֵ
    }
}

float MM::GetFloat(int address, const std::vector<int>& offsets) {
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // ��ַ + ƫ��
    try {
        uintptr_t finalAddress = ApplyOffsets(_address, offsets);  // ͨ��ƫ�����������յ�ַ
        if (!finalAddress) {  // ������յ�ַ�Ƿ���Ч
            throw std::runtime_error("Final address is invalid in GetFloat.");
        }
        float* currentValue = (float*)finalAddress;  // ��ȡָ��Ŀ��ֵ��ָ��
        return *currentValue;
    }
    catch (const std::exception& e) {
        print("Error in GetFloat: ", e.what());
        return 0.0f;  // ����Ĭ��ֵ
    }
}
