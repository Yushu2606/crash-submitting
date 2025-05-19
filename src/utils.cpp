#include "utils.h"

std::vector<std::string_view> Slice(std::string_view str, std::string_view str2)
{
    size_t previous{};
    auto current = str.find(str2);
    std::vector<std::string_view> slice;
    while (current != std::string_view::npos)
    {
        if (current > previous)
        {
            slice.push_back(str.substr(previous, current - previous));
        }

        previous = current + 1;
        current = str.find(str2, previous);
    }

    if (previous != str.size())
    {
        slice.push_back(str.substr(previous));
    }

    return slice;
}
