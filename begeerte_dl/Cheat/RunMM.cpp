#include "..\pch.h" 
#include "RunMM.h"
#include "Utils.h"
#include "MM.h" 
#include <iostream>
#include "CheatData.h"
#include "Print.h"

SIZE_T GetPEFileSize(uintptr_t moduleBaseAddress) {
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)moduleBaseAddress;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(moduleBaseAddress + dosHeader->e_lfanew);

    // 获取文件大小
    SIZE_T sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;

    return sizeOfImage;
}

// 修改内存值的函数
void OnPresent() {

}