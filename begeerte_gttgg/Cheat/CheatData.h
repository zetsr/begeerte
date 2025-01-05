#ifndef CHEATDATA_H
#define CHEATDATA_H

#include <Windows.h>
#include "..\Sig\sig.hpp"

class CheatData
{
public:
    // 构造函数
    CheatData() : moduleBase(0) {}

    // 获取模块基址
    uintptr_t moduleBaseAddress()
    {
        // 如果 moduleBase 还没有被初始化（第一次调用）
        if (moduleBase == 0)
        {
            // 获取模块基址
            // moduleBase = (uintptr_t)GetModuleHandle(L"Golden Treasure - The Great Green.exe");
            moduleBase = (uintptr_t)GetModuleHandle(L"UnityPlayer.dll");
        }

        return moduleBase;
    }

private:
    uintptr_t moduleBase;  // 缓存的模块基址
};

extern CheatData* g_cheatdata;  // 声明全局指针

#endif // CHEATDATA_H
