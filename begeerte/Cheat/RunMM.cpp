#include "..\pch.h" 
#include "RunMM.h"
#include "Utils.h"
#include "MM.h" 
#include <iostream>

/*
void m_set_int(int address, int offset, int value) {
    uintptr_t _address = (uintptr_t)GetModuleHandle(L"Dragons-Win64-Shipping.exe") + address; // ��ַ + ƫ��
    uintptr_t* finalAddress = (uintptr_t*)_address;  // ��ȡ���յ�ַ��ָ��

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // ͨ��ָ��������յ�ַ

    int* currentValue = (int*)finalAddress;  // ��ȡָ��Ŀ��ֵ��ָ��
    *currentValue = value;
}

void m_set_float(int address, int offset, float value) {
    uintptr_t _address = (uintptr_t)GetModuleHandle(L"Dragons-Win64-Shipping.exe") + address; // ��ַ + ƫ��
    uintptr_t* finalAddress = (uintptr_t*)_address;  // ��ȡ���յ�ַ��ָ��

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // ͨ��ָ��������յ�ַ

    float* currentValue = (float*)finalAddress;  // ��ȡָ��Ŀ��ֵ��ָ��
    *currentValue = value;
}

int m_get_int(int address, int offset) {
    uintptr_t _address = (uintptr_t)GetModuleHandle(L"Dragons-Win64-Shipping.exe") + address; // ��ַ + ƫ��
    uintptr_t* finalAddress = (uintptr_t*)_address;  // ��ȡ���յ�ַ��ָ��

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // ͨ��ָ��������յ�ַ

    int* currentValue = (int*)finalAddress;  // ��ȡָ��Ŀ��ֵ��ָ��
    return *currentValue;
}

int m_get_float(int address, int offset) {
    uintptr_t _address = (uintptr_t)GetModuleHandle(L"Dragons-Win64-Shipping.exe") + address; // ��ַ + ƫ��
    uintptr_t* finalAddress = (uintptr_t*)_address;  // ��ȡ���յ�ַ��ָ��

    finalAddress = (uintptr_t*)((uintptr_t)*finalAddress + offset);  // ͨ��ָ��������յ�ַ

    float* currentValue = (float*)finalAddress;  // ��ȡָ��Ŀ��ֵ��ָ��
    return *currentValue;
}

*/

// �޸� FrameGen �ĺ���
void HookFrameGen(/*HANDLE hProcess, uintptr_t moduleBaseAddress*/) {

    /*
    * 2024/12/31
    * ����������RPM���о��������ڱ�����á�
    * ָ�����ȷʵ�ǳ��ᡣ
    * ������һ��ͨ��������ɨ���ڴ棬��ȷ���Ƿ�ɹ��ˡ���ȷʵ�ܽ����ڴ��д��������Ϊû�еõ���ȷ�������룬�޷���֪���Ƿ������ȷ������
   
    * 2024/12/16
    * ���Դ�����һ�������Ĺ��ӣ����ƺ����Թ�ס��Ϸ����Ⱦ������Ҳ���Ҳ�ȷ����
    * �����ҷ��ִ���һ��Gui������˵ԭ��Ԥ��ĸ������ѡ�

    * 2024/11/16
    * �������°汾Version 1.1.7

    * 2024/11/7
    * �������°汾Version 1.1.4

    * 2024/11/3
    * �������°汾Version 1.1.3
    * Version 1.1.2 ʵ���ϲ�δӰ��֡���ɵ��ڴ��ַ��ƫ�ơ�
    * ��֪��Ϊʲô������δ�޸�������⣬Ҳ�����ǲ�����Ϊ����һ�������󡱣���Ϊ����֮�����벻ͨ�����ܹ��Դ˽��͵����ɡ�
    * Ҳ��Ӧ��ת��sig��Ϊɨ������������ʹ�û�ַ+ƫ�Ƶ���ʽ����Ϊ���߼���ÿ�θ��¶���Ҫ��������
    * ���⣬����Ȼ�޷���ȷ��סd3d12���⵼�¼���������ע��imgui����ʵʱ���ԡ�

    * �����Ŀ��ֱ��͸��...�����ϷҲ��͸�ˡ�
    */

    /* ��Щ����ʱ��

    const uintptr_t firstOffset = 0x082E1CE0; 
    const uintptr_t finalOffset = 0x4;

    HookMemory frameGen(hProcess, moduleBaseAddress, firstOffset, finalOffset);

    int expectedValue = 1; // 1 = On��0 = Off
    int currentValue = frameGen.GetInt();

    if (currentValue != expectedValue) {
        print(("Try set FrameGen to " + std::to_string(expectedValue) +
            " at Offset: " + std::to_string(firstOffset) +
            "+" + std::to_string(finalOffset)).c_str());
        frameGen.SetInt(expectedValue);
    }
    */

    // ��һ�ַ���
    // 48 8B C4 48 89 50 10 55 53 56 41 57 + A0
    // int* p = reinterpret_cast<int*>(((uintptr_t) * ((void**)(moduleBaseAddress + 0x082E1CE0))) + 0x4);

    int address = 0x082E1CE0;
    int offset = 0x4;
    int value = 1;

    if (MM::GetInt(address, offset) != value) {
        MM::SetInt(address, offset, value);
        std::cout << "Try set FrameGen to " << value << " at Offset: " << address << "+" << offset << std::endl;
    }
}

// �޸���Ⱦ�ֱ���
void HookRenderResolution(/*HANDLE hProcess, uintptr_t moduleBaseAddress*/) {
    /*
    û������
    Quality��Ultra Quality��ʵ���𲻴�
    �������������ţ������ֶ���
    */

    /* ��Щ����ʱ��
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
        std::cout << "Try set RenderResolution to " << value << " at Offset: " << address << "+" << offset << std::endl;
    }
}

// �޸��ڴ�ֵ�ĺ���
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