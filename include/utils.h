#pragma once

#include <windows.h>

#include <string>
#include <string_view>
#include <vector>

#include <cpr/cpr.h>

constexpr const char* LIBRARY_VERSION = "1.2.2";
constexpr const wchar_t* TITLE = TEXT("彼阳的汉化组");
inline std::string URL;
inline std::string VERSION;
inline bool IS_TEST_VERSION;

std::vector<std::string_view> Slice(std::string_view, std::string_view);
std::wstring ConvertMultiToWide(LPCCH);
bool CheckRTPInstalled();
void CheckUpdate(cpr::Response);
