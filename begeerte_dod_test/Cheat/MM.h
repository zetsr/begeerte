#ifndef MM_h
#define MM_h

// �����Ҫ��ͷ�ļ�
#include <Windows.h>

class MM
{
public:
    // ����int����ֵ
    static void SetInt(int address, int offset, int value);

    // ��ȡint����ֵ
    static int GetInt(int address, int offset);

    // ����float����ֵ
    static void SetFloat(int address, int offset, float value);

    // ��ȡfloat����ֵ
    static float GetFloat(int address, int offset);

private:

};

#endif // MM_H
