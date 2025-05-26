#include "utils.h"

#include <shellapi.h>

#include <filesystem>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

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

std::wstring ConvertMultiToWide(LPCCH m)
{
    auto len = MultiByteToWideChar(CP_UTF8, 0, m, -1, nullptr, 0);
    std::wstring result(len - 1, NULL);
    MultiByteToWideChar(CP_UTF8, 0, m, -1, result.data(), len);
    return result;
}

int CompareVersion(std::string_view version1, std::string_view version2)
{
    size_t i{}, j{};
    auto n1 = version1.size(), n2 = version2.size();

    while (i < n1 || j < n2)
    {
        long long num1{};
        if (i < n1)
        {
            auto start = i;
            while (i < n1 && version1[i] != '.')
            {
                ++i;
            }

            auto [ptr, ec] = std::from_chars(version1.data() + start, version1.data() + i, num1);
            if (ec != std::errc())
            {
                // Handle parsing error
                return -1;
            }

            ++i;
        }

        long long num2{};
        if (j < n2)
        {
            auto start = j;
            while (j < n2 && version2[j] != '.')
            {
                ++j;
            }

            auto [ptr, ec] = std::from_chars(version2.data() + start, version2.data() + j, num2);
            if (ec != std::errc())
            {
                // Handle parsing error
                return 1;
            }

            ++j;
        }

        if (num1 < num2)
        {
            return -1;
        }

        if (num1 > num2)
        {
            return 1;
        }
    }

    return 0;
}

bool CheckRTPInstalled()
{
    HKEY k;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Enterbrain\\RGSS3\\RTP"), NULL, KEY_READ, &k) != ERROR_SUCCESS)
    {
        return false;
    }

    wchar_t data[MAX_PATH]{};
    DWORD cb = sizeof(data);

    if (RegQueryValueEx(k, TEXT("RPGVXAce"), nullptr, nullptr, reinterpret_cast<LPBYTE>(data), &cb) != ERROR_SUCCESS)
    {
        return false;
    }

    return std::filesystem::exists(data);
}

void CheckUpdate(cpr::Response r)
{
    if (r.error || r.status_code != 200)
    {
        return;
    }

    auto j = nlohmann::json::parse(r.text, nullptr, false, true);
    if (j.is_null() || j.is_discarded())
    {
        return;
    }

    auto k = IS_TEST_VERSION ? "test" : "release";
    if (!j.contains(k))
    {
        return;
    }

    j = j[k];
    if (!j.contains("version"))
    {
        return;
    }

    auto sVersion = j["version"].get<std::string>();
    if (sVersion.empty())
    {
        return;
    }

    if (CompareVersion(VERSION, sVersion) != -1)
    {
        return;
    }

    if (!j.contains("message"))
    {
        return;
    }

    if (!j.contains("url"))
    {
        return;
    }

    std::wstring text(TEXT("您正在游玩的好像并不是最新版本，是否前往发布页查看详细信息？"));
    auto message = j["message"].get<std::string>();
    if (!message.empty())
    {
        auto mText = std::format("发现新版本 {0}！\r\n---\r\n{1}\r\n---\r\n是否前往发布页查看详细信息？", sVersion, message);
        text = ConvertMultiToWide(mText.c_str());
    }

    std::wstring pUrl(TEXT("https://www.bilibili.com/opus/1061813870909194291"));
    auto sUrl = j["url"].get<std::string>();
    if (!sUrl.empty())
    {
        pUrl = ConvertMultiToWide(sUrl.c_str());
    }

    auto hWnd = GetDesktopWindow();
    if (MessageBox(hWnd, text.c_str(), TEXT("发现更新！"), MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
    {
        ShellExecute(hWnd, TEXT("open"), pUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}
