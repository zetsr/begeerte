#include <thread>
#include <atomic>
#include <d3d9.h>
#include <d3d12.h>
#include <Windows.h>
#include "pch.h" 
#include "kiero\kiero.h"
#include "Utils.h"
#include "RunMM.h"
#include "MM.h"
#include <iostream>

std::atomic<bool> running(false); 

void SetupConsole() {
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CONOUT$", "w", stdout);
}

void CleanupConsole() {
    FreeConsole();
}

/* 
void lockMemoryValue() {
    while (running) {
        modifyMemoryValue();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
*/

void OnProcessAttach() {
    SetupConsole();
    print("DLL_PROCESS_ATTACH called.");
    printProcessInfo();
    
    running = true;  
    //std::thread(lockMemoryValue).detach();  
}

void OnShutDown() {
    print("DLL_PROCESS_SHUT_DOWN called.");
    running = false; 

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    CleanupConsole();
}

// 定义 Present 函数
typedef HRESULT(__stdcall* PresentFunc)(IDirect3DDevice9* pDevice, RECT* pSourceRect, RECT* pDestRect, HWND hDestWindowOverride, RGNDATA* pDirtyRegion);
PresentFunc oPresent = nullptr;

// 定义 ExecuteCommandLists 函数
typedef void (__stdcall* ExecuteCommandLists_t)(UINT numCommandLists, ID3D12CommandList* const* ppCommandLists);
ExecuteCommandLists_t oExecuteCommandLists = nullptr;

// Present 钩子 
HRESULT __stdcall hkPresent(IDirect3DDevice9* pDevice, RECT* pSourceRect, RECT* pDestRect, HWND hDestWindowOverride, RGNDATA* pDirtyRegion)
{
    modifyMemoryValue();

    return oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

// ExecuteCommandLists 钩子 
void __stdcall hkExecuteCommandLists(UINT numCommandLists, ID3D12CommandList* const* ppCommandLists)
{
    // 输出参数
    print("ExecuteCommandLists called!");
    print("numCommandLists: ");

    for (UINT i = 0; i < numCommandLists; ++i)
    {
        std::cout << "ppCommandLists[" << i << "] : " << reinterpret_cast<void*>(ppCommandLists[i]) << std::endl;
    }

    // 调用原始函数
    return oExecuteCommandLists(numCommandLists, ppCommandLists);
}

// 绑定钩子
int kieroExampleThread()
{
    int Present = -1; // 初始化 Present 的值
    // int ExecuteCommandLists = 54; // D3D12 中的 ExecuteCommandLists 函数

    if (kiero::init(kiero::RenderType::D3D12) == kiero::Status::Success) {
        Present = 140; // 140 = D3D12 Present
    }
    else if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success){
        Present = 8; // 140 = D3D12 Present
    }
    else if (kiero::init(kiero::RenderType::Vulkan) == kiero::Status::Success) {
        Present = 105; // 105 = Vulkan vkCmdDraw
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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    DisableThreadLibraryCalls(hModule);

    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        OnProcessAttach();
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)kieroExampleThread, NULL, 0, NULL);
        break;

    case DLL_PROCESS_DETACH:
        OnShutDown();
        break;
    }

    return TRUE;

}