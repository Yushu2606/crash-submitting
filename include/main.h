#pragma once

#include <windows.h>

extern "C"
{
    __declspec(dllexport) void on_start(LPCSTR, LPCSTR, bool);
    __declspec(dllexport) void on_error(LPCSTR, LPCSTR, LPCSTR);
}
