#include "..\pch.h" 
#include "RunHook.h"
#include "Utils.h"
#include "HookMemory.h" 
#include <string>    

// �޸� FrameGen �ĺ���
void HookFrameGen(HANDLE hProcess, uintptr_t moduleBaseAddress) {
    /*
    2024/11/3
    : �������°汾Version 1.1.3

    Version 1.1.2 ʵ���ϲ�δӰ��֡���ɵ��ڴ��ַ��ƫ�ơ�
    ��֪��Ϊʲô������δ�޸�������⣬Ҳ�����ǲ�����Ϊ����һ�������󡱣���Ϊ����֮�����벻ͨ�����ܹ��Դ˽��͵����ɡ�
    Ҳ��Ӧ��ת��sig��Ϊɨ������������ʹ�û�ַ+ƫ�Ƶ���ʽ����Ϊ���߼���ÿ�θ��¶���Ҫ��������
    ���⣬����Ȼ�޷���ȷ��סd3d12���⵼�¼���������ע��imgui����ʵʱ���ԡ�
    
    �����Ŀ��ֱ��͸��...�����ϷҲ��͸�ˡ�
    */

    const uintptr_t firstOffset = 0x082F4980; // 0x082F1418 ����
    const uintptr_t finalOffset = 0x4;

    HookMemory frameGen(hProcess, moduleBaseAddress, firstOffset, finalOffset);

    int expectedValue = 1; // Ԥ��ֵ��1������0�ر�
    int currentValue = frameGen.GetInt();

    if (currentValue != expectedValue) {
        print(("Try set FrameGen to " + std::to_string(expectedValue) +
            " at Offset: " + std::to_string(firstOffset) +
            "+" + std::to_string(finalOffset)).c_str());
        frameGen.SetInt(expectedValue);
    }
}

// �޸���Ⱦ�ֱ���
void HookRenderResolution(HANDLE hProcess, uintptr_t moduleBaseAddress) {
    /*
    û������
    Quality��Ultra Quality��ʵ���𲻴�
    */
    const uintptr_t firstOffset = 0x07F7A828; // �ѹ�ʱ
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

// �޸��ڴ�ֵ�ĺ���
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