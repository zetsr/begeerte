#ifndef UTILS_H
#define UTILS_H

#include <string>

extern "C" __declspec(dllexport) void print(const char* message);
std::string tcharToString(const TCHAR* tcharStr);
void printProcessInfo();

#endif // UTILS_H
#pragma once
