#include <windows.h>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <pdh.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <ShellScalingApi.h>
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "user32.lib")

typedef BOOL(WINAPI* SETPROCESSDPIAWAREAWARE)();
typedef HRESULT(WINAPI* SETPROCESSDPIAWARENESS)(PROCESS_DPI_AWARENESS);

namespace Constants {
    const COLORREF COLOR_MAIN = RGB(255, 100, 0);
    const COLORREF COLOR_ALT = RGB(255, 255, 255);
    const COLORREF COLOR_BG = RGB(0, 0, 0);
    const int INIT_X = 100;
    const int INIT_Y = 100;
    const int INIT_WIDTH = 300;
    const int INIT_HEIGHT = 50;
    const int TEXT_PADDING_X = 10;
    const int TEXT_PADDING_Y = 8;
    const int ROUND_RADIUS = 10;
    const int BASE_FONT_SIZE = 16;
    const int SMALL_FONT_SIZE = 10;
    const std::string TEXT_SEPARATOR = "     ";
    const float SMALL_TEXT_SPACING = 0.5f;

    struct TextFormat {
        static constexpr const char* NAME_TEXT = "begeerte";
        static constexpr const char* PERCENT_SUFFIX = "%";
        static constexpr const char* CPU_LABEL = "CPU";
        static constexpr const char* GHZ_SUFFIX = "GHz";
    };

    const UINT WM_NOTIFYICON = WM_USER + 1;
    const UINT ID_TRAY_APP_ICON = 1;
    const UINT IDM_EXIT = 1001;
}

static float dpiScale = 1.0f;

float GetDPIScale(HMONITOR hMonitor = NULL) {
    UINT dpiX, dpiY;
    if (!hMonitor) {
        hMonitor = MonitorFromWindow(NULL, MONITOR_DEFAULTTOPRIMARY);
    }
    GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
    return static_cast<float>(dpiX) / 96.0f;
}

int ScaleValue(int value) {
    return static_cast<int>(value * dpiScale);
}

struct RectData {
    int x = ScaleValue(Constants::INIT_X);
    int y = ScaleValue(Constants::INIT_Y);
    int width = ScaleValue(Constants::INIT_WIDTH);
    int height = ScaleValue(Constants::INIT_HEIGHT);
    bool isDragging = false;
    POINT dragStart = { 0, 0 };
};

struct SystemInfo {
    double cpuUsage = 0.0;
    double cpuFreq = 0.0;
    std::string timeStr;
};

class CpuMonitor {
private:
    PDH_HQUERY queryUsage = NULL;
    PDH_HCOUNTER counterUsage = NULL;
    PDH_HQUERY queryPerf = NULL;
    PDH_HCOUNTER counterPerf = NULL;
    bool initialized = false;

public:
    CpuMonitor() {
        // Initialize CPU usage query
        if (PdhOpenQuery(NULL, 0, &queryUsage) == ERROR_SUCCESS) {
            if (PdhAddCounter(queryUsage, L"\\Processor(_Total)\\% Processor Time", 0, &counterUsage) == ERROR_SUCCESS) {
                PdhCollectQueryData(queryUsage);
                initialized = true;
            }
        }

        // Initialize CPU performance query
        if (PdhOpenQuery(NULL, 0, &queryPerf) == ERROR_SUCCESS) {
            if (PdhAddCounter(queryPerf, L"\\Processor Information(_Total)\\% Processor Performance", 0, &counterPerf) == ERROR_SUCCESS) {
                PdhCollectQueryData(queryPerf);
            }
            else {
                OutputDebugStringA("Failed to add Processor Performance counter\n");
                PdhCloseQuery(queryPerf);
                queryPerf = NULL;
            }
        }
    }

    ~CpuMonitor() {
        if (queryUsage) PdhCloseQuery(queryUsage);
        if (queryPerf) PdhCloseQuery(queryPerf);
    }

    double GetCpuUsage() {
        if (!initialized || !queryUsage) return 0.0;
        PDH_FMT_COUNTERVALUE value = { 0 };
        PdhCollectQueryData(queryUsage);
        PdhGetFormattedCounterValue(counterUsage, PDH_FMT_DOUBLE, NULL, &value);
        return value.doubleValue;
    }

    double GetCpuPerformancePercentage() {
        if (!queryPerf) return 100.0;  // Default to 100% if counter unavailable
        PDH_FMT_COUNTERVALUE value = { 0 };
        PdhCollectQueryData(queryPerf);
        PdhGetFormattedCounterValue(counterPerf, PDH_FMT_DOUBLE, NULL, &value);
        return value.doubleValue;
    }
};

class WmiCpuFreq {
private:
    IWbemLocator* pLoc = NULL;
    IWbemServices* pSvc = NULL;
    bool initialized = false;

public:
    WmiCpuFreq() {
        HRESULT hres = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hres)) {
            OutputDebugStringA("CoInitializeEx failed\n");
            return;
        }

        hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
        if (FAILED(hres)) {
            OutputDebugStringA("CoCreateInstance failed\n");
            CoUninitialize();
            return;
        }

        hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
        if (FAILED(hres)) {
            OutputDebugStringA("ConnectServer failed\n");
            pLoc->Release();
            CoUninitialize();
            return;
        }

        hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
        if (FAILED(hres)) {
            OutputDebugStringA("CoSetProxyBlanket failed\n");
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return;
        }

        initialized = true;
    }

    ~WmiCpuFreq() {
        if (initialized) {
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
        }
    }

    double GetBaseFrequency() {
        if (!initialized) {
            OutputDebugStringA("WMI not initialized\n");
            return 0.0;
        }

        IEnumWbemClassObject* pEnumerator = NULL;
        HRESULT hres = pSvc->ExecQuery(_bstr_t(L"WQL"),
            _bstr_t(L"SELECT CurrentClockSpeed FROM Win32_Processor"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator);

        if (FAILED(hres)) {
            char buffer[100];
            sprintf_s(buffer, "ExecQuery failed with HRESULT: 0x%X\n", hres);
            OutputDebugStringA(buffer);
            return 0.0;
        }

        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;
        double freq = 0.0;

        if (pEnumerator) {
            hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (uReturn) {
                VARIANT vtProp;
                VariantInit(&vtProp);
                hres = pclsObj->Get(L"CurrentClockSpeed", 0, &vtProp, 0, 0);
                if (SUCCEEDED(hres)) {
                    switch (vtProp.vt) {
                    case VT_UI4:
                        freq = vtProp.uintVal / 1000.0;
                        break;
                    case VT_I4:
                        freq = vtProp.lVal / 1000.0;
                        break;
                    case VT_BSTR:
                        freq = _wtof(vtProp.bstrVal) / 1000.0;
                        break;
                    default:
                        OutputDebugStringA("Unsupported variant type for base frequency\n");
                        freq = 0.0;
                        break;
                    }
                    if (freq > 0.0) {
                        char buffer[100];
                        sprintf_s(buffer, "Base Frequency: %.1f GHz\n", freq);
                        OutputDebugStringA(buffer);
                    }
                    VariantClear(&vtProp);
                }
                pclsObj->Release();
            }
            pEnumerator->Release();
        }

        return freq;
    }
};

void AddTrayIcon(HWND hwnd, HINSTANCE hInstance) {
    NOTIFYICONDATA nid = { 0 };
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = Constants::ID_TRAY_APP_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = Constants::WM_NOTIFYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    lstrcpy(nid.szTip, L"System Overlay");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RemoveTrayIcon(HWND hwnd) {
    NOTIFYICONDATA nid = { 0 };
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = Constants::ID_TRAY_APP_ICON;
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

void UpdateSystemInfo(SystemInfo& info, CpuMonitor& cpuMonitor, WmiCpuFreq& cpuFreqMonitor) {
    info.cpuUsage = cpuMonitor.GetCpuUsage();
    double baseFreq = cpuFreqMonitor.GetBaseFrequency();
    double perfPercentage = cpuMonitor.GetCpuPerformancePercentage();
    info.cpuFreq = baseFreq * (perfPercentage / 100.0);

    char buffer[100];
    sprintf_s(buffer, "Actual Frequency: %.1f GHz (Base: %.1f GHz, Perf: %.1f%%)\n",
        info.cpuFreq, baseFreq, perfPercentage);
    OutputDebugStringA(buffer);

    auto now = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t(now);
    tm local_tm = { 0 };
    localtime_s(&local_tm, &tt);
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << local_tm.tm_hour << ":"
        << std::setfill('0') << std::setw(2) << local_tm.tm_min << ":"
        << std::setfill('0') << std::setw(2) << local_tm.tm_sec;
    info.timeStr = oss.str();
}

std::string GenerateDisplayText(const SystemInfo& info) {
    std::ostringstream oss;
    oss << Constants::TextFormat::NAME_TEXT << Constants::TEXT_SEPARATOR
        << static_cast<int>(info.cpuUsage) << Constants::TextFormat::PERCENT_SUFFIX
        << Constants::TextFormat::CPU_LABEL << Constants::TEXT_SEPARATOR
        << std::fixed << std::setprecision(1) << info.cpuFreq << Constants::TextFormat::GHZ_SUFFIX
        << Constants::TEXT_SEPARATOR << info.timeStr;
    return oss.str();
}

void AdjustRectSize(HDC hdc, const RectData& rect, const SystemInfo& info) {
    HFONT hFontNormal = CreateFont(ScaleValue(Constants::BASE_FONT_SIZE), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT hFontSmall = CreateFont(ScaleValue(Constants::SMALL_FONT_SIZE), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");

    std::ostringstream freqStream;
    freqStream << std::fixed << std::setprecision(1) << info.cpuFreq;

    struct TextSegment {
        std::string text;
        bool useSmallFont;
    };

    TextSegment segments[] = {
        {Constants::TextFormat::NAME_TEXT, false},
        {Constants::TEXT_SEPARATOR, false},
        {std::to_string(static_cast<int>(info.cpuUsage)) + Constants::TextFormat::PERCENT_SUFFIX, false},
        {Constants::TextFormat::CPU_LABEL, true},
        {Constants::TEXT_SEPARATOR, false},
        {freqStream.str(), false},
        {Constants::TextFormat::GHZ_SUFFIX, true},
        {Constants::TEXT_SEPARATOR, false},
        {info.timeStr, false}
    };

    int totalWidth = 0;
    int maxHeight = 0;
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFontNormal);

    for (const auto& segment : segments) {
        SelectObject(hdc, segment.useSmallFont ? hFontSmall : hFontNormal);
        SIZE size = { 0 };
        GetTextExtentPoint32A(hdc, segment.text.c_str(), static_cast<int>(segment.text.length()), &size);
        totalWidth += size.cx;
        if (segment.useSmallFont) {
            if (segment.text == Constants::TextFormat::CPU_LABEL ||
                segment.text == Constants::TextFormat::GHZ_SUFFIX) {
                SIZE spaceSize = { 0 };
                GetTextExtentPoint32A(hdc, " ", 1, &spaceSize);
                totalWidth += static_cast<int>(spaceSize.cx * Constants::SMALL_TEXT_SPACING);
            }
        }
        maxHeight = max(maxHeight, size.cy);
    }

    const_cast<RectData&>(rect).width = totalWidth + ScaleValue(Constants::TEXT_PADDING_X * 2);
    const_cast<RectData&>(rect).height = maxHeight + ScaleValue(Constants::TEXT_PADDING_Y * 2);

    SelectObject(hdc, hOldFont);
    DeleteObject(hFontNormal);
    DeleteObject(hFontSmall);
}

void UpdateWindowSizeAndPosition(HWND hwnd, HDC hdc, const RectData& rect, const SystemInfo& info) {
    AdjustRectSize(hdc, rect, info);
    SetWindowPos(hwnd, NULL, rect.x, rect.y, rect.width, rect.height, SWP_NOZORDER | SWP_NOACTIVATE);
}

void DrawContent(HDC hdc, const RectData& rect, const SystemInfo& info) {
    HRGN hRgn = CreateRoundRectRgn(0, 0, rect.width, rect.height,
        ScaleValue(Constants::ROUND_RADIUS),
        ScaleValue(Constants::ROUND_RADIUS));
    SelectClipRgn(hdc, hRgn);

    HBRUSH bgBrush = CreateSolidBrush(Constants::COLOR_BG);
    RECT bgRect = { 0, 0, rect.width, rect.height };
    FillRect(hdc, &bgRect, bgBrush);

    HFONT hFontNormal = CreateFont(ScaleValue(Constants::BASE_FONT_SIZE), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT hFontSmall = CreateFont(ScaleValue(Constants::SMALL_FONT_SIZE), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");

    SetBkMode(hdc, TRANSPARENT);

    std::ostringstream freqStream;
    freqStream << std::fixed << std::setprecision(1) << info.cpuFreq;

    struct TextSegment {
        std::string text;
        COLORREF color;
        bool useSmallFont;
    };

    TextSegment segments[] = {
        {Constants::TextFormat::NAME_TEXT, Constants::COLOR_MAIN, false},
        {Constants::TEXT_SEPARATOR, Constants::COLOR_MAIN, false},
        {std::to_string(static_cast<int>(info.cpuUsage)) + Constants::TextFormat::PERCENT_SUFFIX, Constants::COLOR_MAIN, false},
        {Constants::TextFormat::CPU_LABEL, Constants::COLOR_ALT, true},
        {Constants::TEXT_SEPARATOR, Constants::COLOR_MAIN, false},
        {freqStream.str(), Constants::COLOR_MAIN, false},
        {Constants::TextFormat::GHZ_SUFFIX, Constants::COLOR_ALT, true},
        {Constants::TEXT_SEPARATOR, Constants::COLOR_MAIN, false},
        {info.timeStr, Constants::COLOR_ALT, false}
    };

    int currentX = ScaleValue(Constants::TEXT_PADDING_X);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFontNormal);

    SIZE normalSize = { 0 };
    SelectObject(hdc, hFontNormal);
    GetTextExtentPoint32A(hdc, "A", 1, &normalSize);
    SIZE smallSize = { 0 };
    SelectObject(hdc, hFontSmall);
    GetTextExtentPoint32A(hdc, "A", 1, &smallSize);

    int baseY = ScaleValue(Constants::TEXT_PADDING_Y);

    for (const auto& segment : segments) {
        SelectObject(hdc, segment.useSmallFont ? hFontSmall : hFontNormal);

        int yPos = baseY;
        if (segment.useSmallFont) {
            yPos = baseY;
        }

        SetTextColor(hdc, segment.color);
        TextOutA(hdc, currentX, yPos, segment.text.c_str(), static_cast<int>(segment.text.length()));

        SIZE size = { 0 };
        GetTextExtentPoint32A(hdc, segment.text.c_str(), static_cast<int>(segment.text.length()), &size);
        currentX += size.cx;

        if (segment.text == Constants::TextFormat::CPU_LABEL ||
            segment.text == Constants::TextFormat::GHZ_SUFFIX) {
            SIZE spaceSize = { 0 };
            GetTextExtentPoint32A(hdc, " ", 1, &spaceSize);
            currentX += static_cast<int>(spaceSize.cx * Constants::SMALL_TEXT_SPACING);
        }
    }

    SelectObject(hdc, hOldFont);
    DeleteObject(hFontNormal);
    DeleteObject(hFontSmall);
    DeleteObject(bgBrush);
    DeleteObject(hRgn);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static RectData rect;
    static SystemInfo info;
    static CpuMonitor cpuMonitor;
    static WmiCpuFreq cpuFreqMonitor;

    switch (msg) {
    case WM_CREATE:
        dpiScale = GetDPIScale();
        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
        SetTimer(hwnd, 1, 1000, NULL);
        UpdateSystemInfo(info, cpuMonitor, cpuFreqMonitor);
        AddTrayIcon(hwnd, GetModuleHandle(NULL));
        {
            HDC hdc = GetDC(hwnd);
            UpdateWindowSizeAndPosition(hwnd, hdc, rect, info);
            ReleaseDC(hwnd, hdc);
        }
        break;

    case WM_TIMER:
        UpdateSystemInfo(info, cpuMonitor, cpuFreqMonitor);
        {
            HDC hdc = GetDC(hwnd);
            UpdateWindowSizeAndPosition(hwnd, hdc, rect, info);
            ReleaseDC(hwnd, hdc);
        }
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_MOUSEMOVE:
        if (rect.isDragging) {
            POINT pt = { 0 };
            GetCursorPos(&pt);
            rect.x = pt.x - rect.dragStart.x;
            rect.y = pt.y - rect.dragStart.y;
            SetWindowPos(hwnd, NULL, rect.x, rect.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;

    case WM_LBUTTONDOWN: {
        POINT pt = { 0 };
        GetCursorPos(&pt);
        RECT winRect = { rect.x, rect.y, rect.x + rect.width, rect.y + rect.height };
        if (PtInRect(&winRect, pt)) {
            rect.isDragging = true;
            rect.dragStart = { pt.x - rect.x, pt.y - rect.y };
            SetCapture(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    }

    case WM_LBUTTONUP:
        if (rect.isDragging) {
            rect.isDragging = false;
            ReleaseCapture();
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps = { 0 };
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawContent(hdc, rect, info);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DPICHANGED: {
        dpiScale = GetDPIScale();
        RECT* const prcNewWindow = (RECT*)lParam;
        SetWindowPos(hwnd, NULL,
            prcNewWindow->left,
            prcNewWindow->top,
            prcNewWindow->right - prcNewWindow->left,
            prcNewWindow->bottom - prcNewWindow->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }

    case Constants::WM_NOTIFYICON:
        if (LOWORD(lParam) == WM_RBUTTONDOWN) {
            POINT pt = { 0 };
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, Constants::IDM_EXIT, L"Exit");

            MENUINFO mi = { 0 };
            mi.cbSize = sizeof(MENUINFO);
            mi.fMask = MIM_STYLE;
            GetMenuInfo(hMenu, &mi);
            SetMenuInfo(hMenu, &mi);

            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON,
                pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == Constants::IDM_EXIT) {
            DestroyWindow(hwnd);
        }
        break;

    case WM_DESTROY:
        RemoveTrayIcon(hwnd);
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    HMODULE hUser32 = LoadLibrary(L"user32.dll");
    if (hUser32) {
        SETPROCESSDPIAWAREAWARE SetProcessDpiAware =
            (SETPROCESSDPIAWAREAWARE)GetProcAddress(hUser32, "SetProcessDpiAware");

        HMODULE hShcore = LoadLibrary(L"Shcore.dll");
        if (hShcore) {
            SETPROCESSDPIAWARENESS SetProcessDpiAwareness =
                (SETPROCESSDPIAWARENESS)GetProcAddress(hShcore, "SetProcessDpiAwareness");
            if (SetProcessDpiAwareness) {
                SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
            }
            else if (SetProcessDpiAware) {
                SetProcessDpiAware();
            }
            FreeLibrary(hShcore);
        }
        else if (SetProcessDpiAware) {
            SetProcessDpiAware();
        }
        FreeLibrary(hUser32);
    }

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OverlayWindow";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"OverlayWindow",
        L"System Overlay",
        WS_POPUP | WS_VISIBLE,
        ScaleValue(Constants::INIT_X), ScaleValue(Constants::INIT_Y),
        ScaleValue(Constants::INIT_WIDTH), ScaleValue(Constants::INIT_HEIGHT),
        NULL, NULL, hInstance, NULL);

    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}