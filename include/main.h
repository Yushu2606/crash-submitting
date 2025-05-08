#pragma once

#include <windows.h>

typedef int(WINAPI *MESSAGEBOXW)(HWND, LPCWSTR, LPCWSTR, UINT);

inline MESSAGEBOXW fpMessageBoxW = NULL;

extern "C" __declspec(dllexport) BOOL start_hook(LPCSTR);
