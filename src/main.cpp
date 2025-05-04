#include "main.h"

MESSAGEBOXW fpMessageBoxW = NULL;
const char* URL = NULL;

static int WINAPI DetourMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
    if (URL == NULL) {
        return fpMessageBoxW(hWnd, lpText, lpCaption, uType);
    }

    try {
        nlohmann::json j{ {"title", ConvertLPCWSTRToString(lpCaption)}, {"text", ConvertLPCWSTRToString(lpText)}};
        cpr::AsyncResponse p = cpr::PostAsync(cpr::Url(URL), cpr::Body(j.dump()));
        int result = fpMessageBoxW(hWnd, lpText, lpCaption, uType);
        p.wait();
        return result;
    } catch (std::exception& e) {
        MessageBoxA(hWnd, e.what(), "SUBMIT ERROR", uType);
        return fpMessageBoxW(hWnd, lpText, lpCaption, uType);
    }
}

static std::string ConvertLPCWSTRToString(LPCWSTR lpcwstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, lpcwstr, -1, NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, lpcwstr, -1, &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

BOOL start_hook(const char* url)
{
    URL = url;

    if (MH_Initialize() != MH_OK)
    {
        return 1;
    }

    if (MH_CreateHook(&MessageBoxW, &DetourMessageBoxW, reinterpret_cast<LPVOID*>(&fpMessageBoxW)) != MH_OK)
    {
        return 1;
    }

    if (MH_EnableHook(&MessageBoxW) != MH_OK)
    {
        return 1;
    }

    return 0;
}
