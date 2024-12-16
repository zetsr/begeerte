#ifndef HOOK_MEMORY_H
#define HOOK_MEMORY_H

#include <windows.h>
#include <string>

// 声明一个 HookMemory 结构
struct HookMemory {
    HANDLE hProcess;
    uintptr_t moduleBaseAddress;
    uintptr_t firstOffset;
    uintptr_t finalOffset;

    HookMemory(HANDLE process, uintptr_t base, uintptr_t firstOffset, uintptr_t finalOffset);
    int GetInt();
    float GetFloat();
    void SetInt(int value);
    void SetFloat(float value);
};

#endif // HOOK_MEMORY_H
