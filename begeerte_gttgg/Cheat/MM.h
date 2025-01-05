#ifndef MM_H
#define MM_H

#include <Windows.h>
#include <vector>

class MM
{
public:
    // 设置int类型值
    static void SetInt(int address, const std::vector<int>& offsets, int value);

    // 获取int类型值
    static int GetInt(int address, const std::vector<int>& offsets);

    // 设置float类型值
    static void SetFloat(int address, const std::vector<int>& offsets, float value);

    // 获取float类型值
    static float GetFloat(int address, const std::vector<int>& offsets);

private:
    // 该模块的基址（假设基址是已知的）
    static constexpr int baseAddress = 0x12345678; // 请替换为实际的基址
};

#endif // MM_H
