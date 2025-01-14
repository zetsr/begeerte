#ifndef PRINT_H
#define PRINT_H

#include <iostream>
#include <string>

// ���� NoValue �ṹ��
struct NoValue {};

// �����������������Ϊ��ʱ�����ݹ飨�������壬��Ϊ����һ���򵥵ĺ�����
inline void print_helper() {}

// �ݹ�����������һ��������Ȼ��ݹ鴦��ʣ�����
template <typename T, typename... Args>
inline void print_helper(const T& first, Args... args) {
    std::cout << first; // �����һ������
    if constexpr (sizeof...(args) > 0) { // ����Ƿ���ʣ�����
        //std::cout << " "; // �ڲ���֮����ӿո�
        print_helper(args...); // �ݹ鴦��ʣ�����
    }
}

// �ɱ����ģ��� print ����
template <typename... Args>
inline void print(Args... args) {
    print_helper(args...); // ���� print_helper �����������
    std::cout << std::endl; // �����в���֮����ӻ��з�
}

// �ػ� print_helper �Ժ��� NoValue ���͵Ĳ������������壬��Ϊ����һ���򵥵��ػ���
inline void print_helper(const NoValue&) {}

#endif // PRINT_H