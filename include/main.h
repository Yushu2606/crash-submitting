#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdexcept>
#include <minhook.h>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>

typedef int (WINAPI* MESSAGEBOXW)(HWND, LPCWSTR, LPCWSTR, UINT);

extern MESSAGEBOXW fpMessageBoxW;
extern const char* URL;

int WINAPI DetourMessageBoxW(HWND, LPCWSTR, LPCWSTR, UINT);
std::string ConvertLPCWSTRToString(LPCWSTR);
extern "C"
{
    __declspec(dllexport) BOOL start_hook(const char*);
}
