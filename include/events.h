#pragma once

extern "C"
{
    __declspec(dllexport) bool on_start(const char*, const char*, bool);
    __declspec(dllexport) void on_error(const char*, const char*, const char*);
}
