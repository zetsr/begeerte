#include "pch.h" 
#include "HookMemory.h"
#include "Utils.h" // ���� Utils.h ��ʹ�� print ����

// HookMemory �Ĺ��캯��
HookMemory::HookMemory(HANDLE process, uintptr_t base, uintptr_t frameOffset, uintptr_t finalOffset)
    : hProcess(process), moduleBaseAddress(base), frameGenOffset(frameOffset), finalOffset(finalOffset) {}

// ��ȡ��ǰֵ�ĺ���
int HookMemory::Get() {
    uintptr_t address = moduleBaseAddress + frameGenOffset; // ����ַ + ֡ƫ��
    uintptr_t finalAddress;

    // ��ȡ�ڴ��ַ
    if (!ReadProcessMemory(hProcess, (BYTE*)address, &finalAddress, sizeof(finalAddress), NULL)) {
        print(("Failed to read memory for Get() at address: " + std::to_string(address) + ". Returning 0.").c_str());
        return 0;
    }

    finalAddress += finalOffset; // ��������ƫ��
    int currentValue;
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

// ������ֵ�ĺ���
void HookMemory::Set(int value) {
    uintptr_t address = moduleBaseAddress + frameGenOffset; // ����ַ + ֡ƫ��
    uintptr_t finalAddress;

    // ��ȡ�ڴ��ַ
    if (!ReadProcessMemory(hProcess, (BYTE*)address, &finalAddress, sizeof(finalAddress), NULL)) {
        print(("Failed to read memory for Set() at address: " + std::to_string(address) + ".").c_str());
        return;
    }

    finalAddress += finalOffset; // ��������ƫ��
    // д����ֵ
    if (!WriteProcessMemory(hProcess, (BYTE*)finalAddress, &value, sizeof(value), NULL)) {
        print(("Failed to write memory at address: " + std::to_string(finalAddress) +
            " (base: " + std::to_string(moduleBaseAddress) + ", frame offset: " + std::to_string(frameGenOffset) + ", final offset: " + std::to_string(finalOffset) + ").").c_str());
    }
    else {
        print(("Memory modified to: " + std::to_string(value) + " at address: " + std::to_string(finalAddress) +
            " (base: " + std::to_string(moduleBaseAddress) + ", frame offset: " + std::to_string(frameGenOffset) + ", final offset: " + std::to_string(finalOffset) + ").").c_str());
    }
}