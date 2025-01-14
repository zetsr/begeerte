#ifndef MM_H
#define MM_H

#include <Windows.h>
#include <vector>

class MM
{
public:
    // ����int����ֵ
    static void SetInt(int address, const std::vector<int>& offsets, int value);

    // ��ȡint����ֵ
    static int GetInt(int address, const std::vector<int>& offsets);

    // ����float����ֵ
    static void SetFloat(int address, const std::vector<int>& offsets, float value);

    // ��ȡfloat����ֵ
    static float GetFloat(int address, const std::vector<int>& offsets);

private:
    // ��ģ��Ļ�ַ�������ַ����֪�ģ�
    static constexpr int baseAddress = 0x12345678; // ���滻Ϊʵ�ʵĻ�ַ
};

#endif // MM_H
