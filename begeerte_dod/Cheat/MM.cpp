#include "MM.h"
#include "CheatData.h"

void MM::SetInt(int address, int offset, int value)
{
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // ��ַ + ƫ��
    uintptr_t* finalAddress = (uintptr_t*)_address;  // ��ȡ���յ�ַ��ָ��

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // ͨ��ָ��������յ�ַ

    int* currentValue = (int*)finalAddress;  // ��ȡָ��Ŀ��ֵ��ָ��
    *currentValue = value;
}

void MM::SetFloat(int address, int offset, float value)
{
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // ��ַ + ƫ��
    uintptr_t* finalAddress = (uintptr_t*)_address;  // ��ȡ���յ�ַ��ָ��

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // ͨ��ָ��������յ�ַ

    float* currentValue = (float*)finalAddress;  // ��ȡָ��Ŀ��ֵ��ָ��
    *currentValue = value;
}

int MM::GetInt(int address, int offset)
{
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // ��ַ + ƫ��
    uintptr_t* finalAddress = (uintptr_t*)_address;  // ��ȡ���յ�ַ��ָ��

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // ͨ��ָ��������յ�ַ

    int* currentValue = (int*)finalAddress;  // ��ȡָ��Ŀ��ֵ��ָ��
    return *currentValue;
}

float MM::GetFloat(int address, int offset)
{
    uintptr_t _address = g_cheatdata->moduleBaseAddress() + address; // ��ַ + ƫ��
    uintptr_t* finalAddress = (uintptr_t*)_address;  // ��ȡ���յ�ַ��ָ��

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // ͨ��ָ��������յ�ַ

    float* currentValue = (float*)finalAddress;  // ��ȡָ��Ŀ��ֵ��ָ��
    return *currentValue;
}