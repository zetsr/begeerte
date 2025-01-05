#ifndef PRINT_H
#define PRINT_H

#include <iostream>
#include <string>

// 声明 NoValue 结构体
struct NoValue {};

// 基础情况：当参数包为空时结束递归（内联定义，因为这是一个简单的函数）
inline void print_helper() {}

// 递归情况：处理第一个参数，然后递归处理剩余参数
template <typename T, typename... Args>
inline void print_helper(const T& first, Args... args) {
    std::cout << first; // 输出第一个参数
    if constexpr (sizeof...(args) > 0) { // 检查是否还有剩余参数
        //std::cout << " "; // 在参数之间添加空格
        print_helper(args...); // 递归处理剩余参数
    }
}

// 可变参数模板的 print 函数
template <typename... Args>
inline void print(Args... args) {
    print_helper(args...); // 调用 print_helper 来处理参数包
    std::cout << std::endl; // 在所有参数之后添加换行符
}

// 特化 print_helper 以忽略 NoValue 类型的参数（内联定义，因为这是一个简单的特化）
inline void print_helper(const NoValue&) {}

#endif // PRINT_H