#ifndef MM_h
#define MM_h

// 引入必要的头文件
#include <Windows.h>

class MM
{
public:
    // 设置int类型值
    static void SetInt(int address, int offset, int value);

    // 获取int类型值
    static int GetInt(int address, int offset);

    // 设置float类型值
    static void SetFloat(int address, int offset, float value);

    // 获取float类型值
    static float GetFloat(int address, int offset);

private:
    // 该模块的基址（假设基址是已知的）
    static constexpr int baseAddress = 0x12345678; // 请替换为实际的基址
};

#endif // MM_H
