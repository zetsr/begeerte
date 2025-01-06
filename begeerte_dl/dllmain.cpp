#include <thread>
#include <atomic>
#include <Windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fcntl.h>
#include <io.h>

#include <d3d9.h>
#include <d3d12.h>

#include "pch.h" 
#include "kiero\kiero.h"

#include "Utils.h"
#include "RunMM.h"
#include "MM.h"
#include "Print.h"
#include "Console.h"
#include "Command.h"

// 定义 Present 函数
typedef HRESULT(__stdcall* PresentFunc)(IDirect3DDevice9* pDevice, RECT* pSourceRect, RECT* pDestRect, HWND hDestWindowOverride, RGNDATA* pDirtyRegion);
PresentFunc oPresent = nullptr;

// 定义 ExecuteCommandLists 函数
typedef void(__stdcall* ExecuteCommandLists_t)(UINT numCommandLists, ID3D12CommandList* const* ppCommandLists);
ExecuteCommandLists_t oExecuteCommandLists = nullptr;

// Present 钩子 
HRESULT __stdcall hkPresent(IDirect3DDevice9* pDevice, RECT* pSourceRect, RECT* pDestRect, HWND hDestWindowOverride, RGNDATA* pDirtyRegion)
{
    OnPresent();
    return oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

// ExecuteCommandLists 钩子 
void __stdcall hkExecuteCommandLists(UINT numCommandLists, ID3D12CommandList* const* ppCommandLists)
{
    // 输出参数
    print("ExecuteCommandLists called!");

    // 调用原始函数
    return oExecuteCommandLists(numCommandLists, ppCommandLists);
}

// 绑定钩子
int kieroExampleThread()
{
    int Present = -1; // 初始化 Present 的值
    int ExecuteCommandLists = 54; // D3D12 中的 ExecuteCommandLists 函数

    if (kiero::init(kiero::RenderType::D3D12) == kiero::Status::Success) {
        Present = 140; // 140 = D3D12 Present
        print("D3D12");
    }
    else if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success){
        Present = 8; // 8 = D3D11 Present
        // Present = 74; // 74 = D3D11 Draw
        print("D3D11");
    }
    else if (kiero::init(kiero::RenderType::Vulkan) == kiero::Status::Success) {
        Present = 105; // 105 = Vulkan vkCmdDraw
        print("Vulkan");
    }
    else {
        print("render init failed!");
    }

    if (Present != -1) {
        // 绑定钩子
        // kiero::bind(ExecuteCommandLists, (void**)&oExecuteCommandLists, hkExecuteCommandLists); // crash

        // 不要使用这种方法
        // oPresent = (PresentFunc)kiero::getMethodsTable()[v1];

        kiero::bind(Present, (void**)&oPresent, hkPresent);
    }

    return 0;
}

void OnProcessAttach() {
    SetupConsole();
    printSystemInfo();
    printProcessInfo();

    RegisterConsoleEventListener(OnConsoleEvent);  // 注册事件监听器

    StartConsoleListening();  // 启动控制台输入监听

    // 启动钩子线程
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)kieroExampleThread, NULL, 0, NULL);
}

void OnShutDown() {
    print("Shutdown!");
    StopConsoleListening();  // 停止控制台输入监听

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    CleanupConsole();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    DisableThreadLibraryCalls(hModule);

    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        OnProcessAttach();
        break;

    case DLL_PROCESS_DETACH:
        OnShutDown();
        break;
    }

    return TRUE;
}