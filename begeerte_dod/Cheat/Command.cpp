#include <regex>
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <cstdlib>

#include "CheatData.h"
#include "Console.h"
#include "Print.h"
#include "MM.h"

// 提取十六进制数并转换为整数
std::vector<int> extract_hex_params(const std::string& input, int count) {
    std::regex hex_regex("0x[0-9a-fA-F]+");
    std::smatch matches;
    std::string::const_iterator searchStart(input.cbegin());
    std::vector<int> params;

    while (std::regex_search(searchStart, input.cend(), matches, hex_regex) && params.size() < count) {
        std::stringstream ss;
        ss << matches[0];
        int value;
        ss >> std::hex >> value;
        params.push_back(value);
        searchStart = matches.suffix().first;
    }

    return params;
}

// 提取普通整数参数
std::vector<int> extract_int_params(const std::string& input, int count) {
    std::regex int_regex("\\b\\d+\\b");
    std::smatch matches;
    std::string::const_iterator searchStart(input.cbegin());
    std::vector<int> params;

    while (std::regex_search(searchStart, input.cend(), matches, int_regex) && params.size() < count) {
        std::stringstream ss;
        ss << matches[0];
        int value;
        ss >> value;
        params.push_back(value);
        searchStart = matches.suffix().first;
    }

    return params;
}

// 提取浮点数参数
std::vector<float> extract_float_params(const std::string& input, int count) {
    std::regex float_regex("[-+]?\\d*\\.?\\d+");
    std::smatch matches;
    std::string::const_iterator searchStart(input.cbegin());
    std::vector<float> params;

    while (std::regex_search(searchStart, input.cend(), matches, float_regex) && params.size() < count) {
        std::stringstream ss;
        ss << matches[0];
        float value;
        ss >> value;
        params.push_back(value);
        searchStart = matches.suffix().first;
    }

    return params;
}

// 处理mm_get_int命令
void o_mm_get_int(const ConsoleEvent& e) {
    std::regex command_regex("^mm_get_int\\s+0x[0-9a-f]+\\s+0x[0-9a-f]+$", std::regex_constants::icase);

    if (std::regex_match(e.text, command_regex)) {
        std::vector<int> params = extract_hex_params(e.text, 2);

        if (params.size() == 2) {
            print(MM::GetInt(params[0], params[1]));
        }
        else {
            print("参数数量不正确");
        }
    }
    else {
        print("无效的命令格式");
    }
}

// 处理mm_set_int命令
void o_mm_set_int(const ConsoleEvent& e) {
    std::regex command_regex("^mm_set_int\\s+0x[0-9a-f]+\\s+0x[0-9a-f]+\\s+\\d+$", std::regex_constants::icase);

    if (std::regex_match(e.text, command_regex)) {
        // 提取前两个十六进制参数
        std::vector<int> hex_params = extract_hex_params(e.text, 2);
        // 提取第三个普通整数参数
        std::vector<int> int_params = extract_int_params(e.text, 1);

        if (hex_params.size() == 2 && int_params.size() == 1) {
            MM::SetInt(hex_params[0], hex_params[1], int_params[0]);
        }
        else {
            print("参数数量不正确");
        }
    }
    else {
        print("无效的命令格式");
    }
}

// 处理mm_get_float命令
void o_mm_get_float(const ConsoleEvent& e) {
    std::regex command_regex("^mm_get_float\\s+0x[0-9a-f]+\\s+0x[0-9a-f]+$", std::regex_constants::icase);

    if (std::regex_match(e.text, command_regex)) {
        std::vector<int> params = extract_hex_params(e.text, 2);

        if (params.size() == 2) {
            print(MM::GetFloat(params[0], params[1]));
        }
        else {
            print("参数数量不正确");
        }
    }
    else {
        print("无效的命令格式");
    }
}

// 处理mm_set_float命令
void o_mm_set_float(const ConsoleEvent& e) {
    std::regex command_regex("^mm_set_float\\s+0x[0-9a-f]+\\s+0x[0-9a-f]+\\s+[-+]?\\d*\\.?\\d+$", std::regex_constants::icase);

    if (std::regex_match(e.text, command_regex)) {
        // 提取前两个十六进制参数
        std::vector<int> hex_params = extract_hex_params(e.text, 2);
        // 提取第三个浮动数参数
        std::vector<float> float_params = extract_float_params(e.text, 1);

        if (hex_params.size() == 2 && float_params.size() == 1) {
            MM::SetFloat(hex_params[0], hex_params[1], float_params[0]);
        }
        else {
            print("参数数量不正确");
        }
    }
    else {
        print("无效的命令格式");
    }
}

void c_FrameGen(const ConsoleEvent& e) {
    std::regex command_regex("^set\\s+FrameGen\\s+(0|1)$", std::regex_constants::icase);

    if (std::regex_match(e.text, command_regex)) {
        // 提取整数参数 (0 或 1)
        std::vector<int> int_params = extract_int_params(e.text, 1);

        if (int_params.size() == 1) {
            bool value = (int_params[0] == 1); // 转换为布尔值
            g_cheatdata->isFrameGen = value;
        }
        else {
            print("参数数量不正确");
        }
    }
    else {
        print("无效的命令格式");
    }
}

void c_RenderResolution(const ConsoleEvent& e) {
    std::regex command_regex("^set\\s+RR\\s+(0|1)$", std::regex_constants::icase);

    if (std::regex_match(e.text, command_regex)) {
        // 提取整数参数 (0 或 1)
        std::vector<int> int_params = extract_int_params(e.text, 1);

        if (int_params.size() == 1) {
            bool value = (int_params[0] == 1); // 转换为布尔值
            g_cheatdata->isRenderResolution = value;
        }
        else {
            print("参数数量不正确");
        }
    }
    else {
        print("无效的命令格式");
    }
}

// 处理clear命令
void o_clear(const ConsoleEvent& e) {
    // 使用system命令根据操作系统清屏
#ifdef _WIN32
    system("cls"); // Windows
#else
    system("clear"); // 类Unix系统 (Linux, macOS)
#endif
}


// 创建命令映射表
std::unordered_map<std::string, std::function<void(const ConsoleEvent&)>> command_map = {
    {"mm_get_int", o_mm_get_int},
    {"mm_set_int", o_mm_set_int},
    {"mm_get_float", o_mm_get_float},
    {"mm_set_float", o_mm_set_float},
    {"set framegen", c_FrameGen},
    {"set rr", c_RenderResolution},
    {"clear", o_clear}
};

// 事件处理函数
void OnConsoleEvent(const ConsoleEvent& e) {
    // 转换输入的命令文本为小写
    std::string input_text = e.text;
    std::transform(input_text.begin(), input_text.end(), input_text.begin(), ::tolower);

    // 遍历命令映射表查找对应的命令并调用处理函数
    for (const auto& pair : command_map) {
        // 将命令映射的命令名也转换为小写进行比较
        std::string command = pair.first;
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);

        if (input_text.find(command) == 0) { // 如果输入的命令是有效的
            pair.second(e); // 调用对应的处理函数
            return;
        }
    }
    print("无效的命令");
}
