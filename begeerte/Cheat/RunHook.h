#ifndef RUN_HOOK_H
#define RUN_HOOK_H

#include <windows.h> 
#include <cstdint> 

// ��������
void HookFrameGen(HANDLE hProcess, uintptr_t moduleBaseAddress);
void HookRenderResolution(HANDLE hProcess, uintptr_t moduleBaseAddress);
void modifyMemoryValue();

#endif // RUN_HOOK_H