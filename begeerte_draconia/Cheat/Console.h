#pragma once
#include <string>
#include <functional>

// �¼��ṹ��
struct ConsoleEvent {
    int id;     // �¼� ID
    std::string text;  // ������ı�����
};

// ��������̨����Ļص���������
using ConsoleEventListener = std::function<void(const ConsoleEvent&)>;

// ����̨��������
void SetupConsole();
void CleanupConsole();
void StartConsoleListening();
void StopConsoleListening();

// ע�������¼�������
void RegisterConsoleEventListener(ConsoleEventListener listener);
