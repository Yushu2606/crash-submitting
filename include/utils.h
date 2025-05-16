#pragma once

#include <windows.h>

#include <string>

#include <cpr/cpr.h>

constexpr const char* LIBRARY_VERSION = "1.2.0";
inline std::string URL;
inline std::string VERSION;

LPCSTR ConvertWideToMulti(LPCWSTR);
LPCWSTR ConvertMultiToWide(LPCSTR);
bool CheckRTPInstalled();
void CheckUpdate(cpr::Response);
void Restart();
