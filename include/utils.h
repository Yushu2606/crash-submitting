#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <cpr/cpr.h>

constexpr const char* LIBRARY_VERSION = "1.2.1";
inline std::string URL;
inline std::string VERSION;
inline bool IS_TEST_VERSION;

std::vector<std::string_view> Slice(std::string_view, std::string_view);
bool CheckRTPInstalled();
void CheckUpdate(cpr::Response);
