#include <thread>
#include <atomic>
#include <Windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fcntl.h>
#include <io.h>

#include "pch.h" 
#include "Utils.h"
#include "Print.h"
#include "Console.h"
#include "Command.h"
#include "Hook.h"

void OnProcessAttach() {
    SetupConsole();
    printSystemInfo();
    printProcessInfo();

    RegisterConsoleEventListener(OnConsoleEvent);  // 注册事件监听器
    StartConsoleListening();  // 启动控制台输入监听

    // 启动钩子线程
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SetupHook, NULL, 0, NULL);
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