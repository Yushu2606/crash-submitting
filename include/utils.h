#pragma once

#include <windows.h>

#include <string>

constexpr const char* LIBRARY_VERSION = "1.1.2";
inline std::string URL;
inline std::string VERSION;

LPCSTR ConvertWideToByte(LPCWSTR);
