#ifndef CHEATDATA_H  
#define CHEATDATA_H

#include <Windows.h>
#include "..\Sig\sig.hpp"

class CheatAddress // ������ά�����л�ַ��ƫ��
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
    class D3D12 // D3D12 �¼�
    {
    public:
        int Present = 140; // Present
    };

    class D3D11  // D3D11 �¼�
    {
    public:
        int Present = 8; // Present
    };

    class Vulkan  // Vulkan �¼�
    {
    public:
        int vkCmdBeginRenderPass = 132; // vkCmdBeginRenderPass
    };

    D3D12* d3d12 = new D3D12();
    D3D11* d3d11 = new D3D11();
    Vulkan* vulkan = new Vulkan();

    // ���������������ڴ�й©
    ~CheatEvent() {
        delete d3d12;
        delete d3d11;
        delete vulkan;
    }
};

class CheatData
{
public:
    // ���캯��
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

    // ��ȡģ���ַ
    uintptr_t moduleBaseAddress()
    {
        // ��� moduleBase ��û�б���ʼ������һ�ε��ã�
        if (moduleBase == 0)
        {
            // ��ȡģ���ַ
            moduleBase = (uintptr_t)GetModuleHandle(L"Draconia-Win64-Shipping.exe");
        }

        return moduleBase;
    }

    CheatAddress* address; // ��ַ����
    CheatOffset* offset; // ƫ�ƹ���
    CheatEvent* event; // �¼�����

private:
    uintptr_t moduleBase; // �����ģ���ַ
};

extern CheatData* g_cheatdata; // ����ȫ��ָ��

#endif // CHEATDATA_H
