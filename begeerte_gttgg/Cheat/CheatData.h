#ifndef CHEATDATA_H
#define CHEATDATA_H

#include <Windows.h>
#include "..\Sig\sig.hpp"

class CheatData
{
public:
    // ���캯��
    CheatData() : moduleBase(0) {}

    // ��ȡģ���ַ
    uintptr_t moduleBaseAddress()
    {
        // ��� moduleBase ��û�б���ʼ������һ�ε��ã�
        if (moduleBase == 0)
        {
            // ��ȡģ���ַ
            // moduleBase = (uintptr_t)GetModuleHandle(L"Golden Treasure - The Great Green.exe");
            moduleBase = (uintptr_t)GetModuleHandle(L"UnityPlayer.dll");
        }

        return moduleBase;
    }

private:
    uintptr_t moduleBase;  // �����ģ���ַ
};

extern CheatData* g_cheatdata;  // ����ȫ��ָ��

#endif // CHEATDATA_H
