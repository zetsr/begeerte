#ifndef CHEATDATA_H  
#define CHEATDATA_H

#include <Windows.h>
#include "..\Sig\sig.hpp"

class CheatAddress // 在这里维护所有基址与偏移
{
public:

};

class CheatOffset
{
public:

};

class CheatEvent
{
public:
    class D3D12 // D3D12 事件
    {
    public:
        int Present = 140; // Present
    };

    class D3D11  // D3D11 事件
    {
    public:
        int Present = 8; // Present
    };

    class Vulkan  // Vulkan 事件
    {
    public:
        int vkCmdBeginRenderPass = 132; // vkCmdBeginRenderPass
    };

    D3D12* d3d12 = new D3D12();
    D3D11* d3d11 = new D3D11();
    Vulkan* vulkan = new Vulkan();

    // 析构函数，避免内存泄漏
    ~CheatEvent() {
        delete d3d12;
        delete d3d11;
        delete vulkan;
    }
};

class CheatData
{
public:
    // 构造函数
    CheatData() : moduleBase(0)
    {
        address = new CheatAddress();
        offset = new CheatOffset();
        event = new CheatEvent();
    }

    ~CheatData()
    {
        delete address;
        delete offset;
        delete event;
    }

    // 获取模块基址
    uintptr_t moduleBaseAddress()
    {
        // 如果 moduleBase 还没有被初始化（第一次调用）
        if (moduleBase == 0)
        {
            // 获取模块基址
            moduleBase = (uintptr_t)GetModuleHandle(L"Draconia-Win64-Shipping.exe");
        }

        return moduleBase;
    }

    CheatAddress* address; // 地址管理
    CheatOffset* offset; // 偏移管理
    CheatEvent* event; // 事件管理

private:
    uintptr_t moduleBase; // 缓存的模块基址
};

extern CheatData* g_cheatdata; // 声明全局指针

#endif // CHEATDATA_H
