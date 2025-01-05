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

// ��ȡʮ����������ת��Ϊ����
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

// ��ȡ��ͨ��������
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

// ��ȡ����������
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

// ����mm_get_int����
void o_mm_get_int(const ConsoleEvent& e) {
    std::regex command_regex("^mm_get_int\\s+0x[0-9a-f]+\\s+0x[0-9a-f]+$", std::regex_constants::icase);

    if (std::regex_match(e.text, command_regex)) {
        std::vector<int> params = extract_hex_params(e.text, 2);

        if (params.size() == 2) {
            print(MM::GetInt(params[0], params[1]));
        }
        else {
            print("������������ȷ");
        }
    }
    else {
        print("��Ч�������ʽ");
    }
}

// ����mm_set_int����
void o_mm_set_int(const ConsoleEvent& e) {
    std::regex command_regex("^mm_set_int\\s+0x[0-9a-f]+\\s+0x[0-9a-f]+\\s+\\d+$", std::regex_constants::icase);

    if (std::regex_match(e.text, command_regex)) {
        // ��ȡǰ����ʮ�����Ʋ���
        std::vector<int> hex_params = extract_hex_params(e.text, 2);
        // ��ȡ��������ͨ��������
        std::vector<int> int_params = extract_int_params(e.text, 1);

        if (hex_params.size() == 2 && int_params.size() == 1) {
            MM::SetInt(hex_params[0], hex_params[1], int_params[0]);
        }
        else {
            print("������������ȷ");
        }
    }
    else {
        print("��Ч�������ʽ");
    }
}

// ����mm_get_float����
void o_mm_get_float(const ConsoleEvent& e) {
    std::regex command_regex("^mm_get_float\\s+0x[0-9a-f]+\\s+0x[0-9a-f]+$", std::regex_constants::icase);

    if (std::regex_match(e.text, command_regex)) {
        std::vector<int> params = extract_hex_params(e.text, 2);

        if (params.size() == 2) {
            print(MM::GetFloat(params[0], params[1]));
        }
        else {
            print("������������ȷ");
        }
    }
    else {
        print("��Ч�������ʽ");
    }
}

// ����mm_set_float����
void o_mm_set_float(const ConsoleEvent& e) {
    std::regex command_regex("^mm_set_float\\s+0x[0-9a-f]+\\s+0x[0-9a-f]+\\s+[-+]?\\d*\\.?\\d+$", std::regex_constants::icase);

    if (std::regex_match(e.text, command_regex)) {
        // ��ȡǰ����ʮ�����Ʋ���
        std::vector<int> hex_params = extract_hex_params(e.text, 2);
        // ��ȡ����������������
        std::vector<float> float_params = extract_float_params(e.text, 1);

        if (hex_params.size() == 2 && float_params.size() == 1) {
            MM::SetFloat(hex_params[0], hex_params[1], float_params[0]);
        }
        else {
            print("������������ȷ");
        }
    }
    else {
        print("��Ч�������ʽ");
    }
}

void c_FrameGen(const ConsoleEvent& e) {
    std::regex command_regex("^set\\s+FrameGen\\s+(0|1)$", std::regex_constants::icase);

    if (std::regex_match(e.text, command_regex)) {
        // ��ȡ�������� (0 �� 1)
        std::vector<int> int_params = extract_int_params(e.text, 1);

        if (int_params.size() == 1) {
            bool value = (int_params[0] == 1); // ת��Ϊ����ֵ
            g_cheatdata->isFrameGen = value;
        }
        else {
            print("������������ȷ");
        }
    }
    else {
        print("��Ч�������ʽ");
    }
}

void c_RenderResolution(const ConsoleEvent& e) {
    std::regex command_regex("^set\\s+RR\\s+(0|1)$", std::regex_constants::icase);

    if (std::regex_match(e.text, command_regex)) {
        // ��ȡ�������� (0 �� 1)
        std::vector<int> int_params = extract_int_params(e.text, 1);

        if (int_params.size() == 1) {
            bool value = (int_params[0] == 1); // ת��Ϊ����ֵ
            g_cheatdata->isRenderResolution = value;
        }
        else {
            print("������������ȷ");
        }
    }
    else {
        print("��Ч�������ʽ");
    }
}

// ����clear����
void o_clear(const ConsoleEvent& e) {
    // ʹ��system������ݲ���ϵͳ����
#ifdef _WIN32
    system("cls"); // Windows
#else
    system("clear"); // ��Unixϵͳ (Linux, macOS)
#endif
}


// ��������ӳ���
std::unordered_map<std::string, std::function<void(const ConsoleEvent&)>> command_map = {
    {"mm_get_int", o_mm_get_int},
    {"mm_set_int", o_mm_set_int},
    {"mm_get_float", o_mm_get_float},
    {"mm_set_float", o_mm_set_float},
    {"set framegen", c_FrameGen},
    {"set rr", c_RenderResolution},
    {"clear", o_clear}
};

// �¼�������
void OnConsoleEvent(const ConsoleEvent& e) {
    // ת������������ı�ΪСд
    std::string input_text = e.text;
    std::transform(input_text.begin(), input_text.end(), input_text.begin(), ::tolower);

    // ��������ӳ�����Ҷ�Ӧ��������ô�����
    for (const auto& pair : command_map) {
        // ������ӳ���������Ҳת��ΪСд���бȽ�
        std::string command = pair.first;
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);

        if (input_text.find(command) == 0) { // ����������������Ч��
            pair.second(e); // ���ö�Ӧ�Ĵ�����
            return;
        }
    }
    print("��Ч������");
}
