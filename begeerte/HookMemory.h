#ifndef HOOK_MEMORY_H
#define HOOK_MEMORY_H

#include <windows.h>
#include <string>

// ����һ�� HookMemory �ṹ
struct HookMemory {
    HANDLE hProcess;
    uintptr_t moduleBaseAddress;
    uintptr_t frameGenOffset;
    uintptr_t finalOffset;

    HookMemory(HANDLE process, uintptr_t base, uintptr_t frameOffset, uintptr_t finalOffset);
    int Get();
    void Set(int value);
};

#endif // HOOK_MEMORY_H
