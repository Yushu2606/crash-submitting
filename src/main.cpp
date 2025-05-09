#include <stdexcept>
#include <minhook.h>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>

#include "main.h"
#include "crash.h"
#include "utils.h"

static int WINAPI DetourMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
    hWnd = NULL;

    try
    {
        std::string title = ConvertLPCWSTRToString(lpCaption);
        std::string text = ConvertLPCWSTRToString(lpText);
        nlohmann::json j{
            {"title", title},
            {"text", text} };

        cpr::Url url(URL);
        cpr::Body body(j.dump());
        cpr::AsyncResponse p = cpr::PostAsync(url, body);
        if (!p.valid())
        {
            MessageBoxA(hWnd, "Is not a vaild post request.", "SUBMIT ERROR", MB_ICONERROR);
        }

        int result = fpMessageBoxW(hWnd, lpText, lpCaption, uType);
        p.wait();
        cpr::Response r = p.get();
        if (r.error)
        {
            MessageBoxA(hWnd, r.error.message.c_str(), "SUBMIT ERROR", MB_ICONERROR);
        }
        return result;
    }
    catch (std::exception& e)
    {
        MessageBoxA(hWnd, e.what(), "SUBMIT ERROR", MB_ICONERROR);
        return fpMessageBoxW(hWnd, lpText, lpCaption, uType);
    }
}

BOOL start_hook(LPCSTR url)
{
    URL = std::string(url);

    AddVectoredExceptionHandler(1, uncatchableExceptionHandler);
    SetUnhandledExceptionFilter(unhandledExceptionFilter);

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
