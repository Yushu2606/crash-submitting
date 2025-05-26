#pragma once

#include <string>
#include <string_view>
#include <vector>

constexpr const char* LIBRARY_VERSION = "1.2.2";
inline std::string URL;
inline std::string VERSION;

std::vector<std::string_view> Slice(std::string_view, std::string_view);
