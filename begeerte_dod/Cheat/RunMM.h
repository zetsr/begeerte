#ifndef RUN_HOOK_H
#define RUN_HOOK_H

#include <windows.h> 
#include <cstdint> 

// º¯ÊýÉùÃ÷
void HookFrameGen(/*HANDLE hProcess, uintptr_t moduleBaseAddress*/);
void HookRenderResolution(/*HANDLE hProcess, uintptr_t moduleBaseAddress*/);
void OnPresent();

#endif // RUN_HOOK_H