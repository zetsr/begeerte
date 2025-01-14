#ifndef HOOK_H
#define HOOK_H

#include <d3d9.h>
#include <d3d12.h>

#include <Print.h>
#include <CheatData.h>
#include "RunMM.h"
#include "..\kiero\kiero.h"

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
    print("numCommandLists: ");

    for (UINT i = 0; i < numCommandLists; ++i)
    {
        print("ppCommandLists[", i, "] : ", reinterpret_cast<void*>(ppCommandLists[i]));
        //std::cout << "ppCommandLists[" << i << "] : " << reinterpret_cast<void*>(ppCommandLists[i]) << std::endl;
    }

    // 调用原始函数
    return oExecuteCommandLists(numCommandLists, ppCommandLists);
}

void SetupHook() {
    int Present = -1; // 初始化 Present

    if (kiero::init(kiero::RenderType::D3D12) == kiero::Status::Success) {
        Present = g_cheatdata->event->d3d12->Present;
        print("D3D12");
    }
    else if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success) {
        Present = g_cheatdata->event->d3d11->Present;
        print("D3D11");
    }
    else if (kiero::init(kiero::RenderType::Vulkan) == kiero::Status::Success) {
        Present = g_cheatdata->event->vulkan->vkCmdBeginRenderPass;
        print("Vulkan");
    }
    else {
        print("render init failed!");
    }

    if (Present != -1) {
        kiero::bind(Present, (void**)&oPresent, hkPresent);
    }
}

#endif // HOOK_H