#pragma once

#include <windows.h>

#include <string>

constexpr const char* LIBRARY_VERSION = "1.1.3";
inline std::string URL;
inline std::string VERSION;

LPCSTR ConvertWideToByte(LPCWSTR);
