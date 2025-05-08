#include "utils.h"

std::string ConvertLPCWSTRToString(LPCWSTR lpcwstr)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, lpcwstr, -1, NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, lpcwstr, -1, &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
