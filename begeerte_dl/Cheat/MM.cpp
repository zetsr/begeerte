#include "MM.h"
#include "CheatData.h"

void MM::SetInt(int address, int offset, int value)
{
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // 基址 + 偏移
    uintptr_t* finalAddress = (uintptr_t*)_address;  // 获取最终地址的指针

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // 通过指针调整最终地址

    int* currentValue = (int*)finalAddress;  // 获取指向目标值的指针
    *currentValue = value;
}

void MM::SetFloat(int address, int offset, float value)
{
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // 基址 + 偏移
    uintptr_t* finalAddress = (uintptr_t*)_address;  // 获取最终地址的指针

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // 通过指针调整最终地址

    float* currentValue = (float*)finalAddress;  // 获取指向目标值的指针
    *currentValue = value;
}

int MM::GetInt(int address, int offset)
{
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // 基址 + 偏移
    uintptr_t* finalAddress = (uintptr_t*)_address;  // 获取最终地址的指针

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // 通过指针调整最终地址

    int* currentValue = (int*)finalAddress;  // 获取指向目标值的指针
    return *currentValue;
}

float MM::GetFloat(int address, int offset)
{
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // 基址 + 偏移
    uintptr_t* finalAddress = (uintptr_t*)_address;  // 获取最终地址的指针

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // 通过指针调整最终地址

    float* currentValue = (float*)finalAddress;  // 获取指向目标值的指针
    return *currentValue;
}