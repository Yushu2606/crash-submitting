#include "utils.h"

#include <shellapi.h>

#include <filesystem>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

LPCSTR ConvertWideToMulti(LPCWSTR w)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    LPSTR s = new char[len];
    WideCharToMultiByte(CP_UTF8, 0, w, -1, s, len, NULL, NULL);
    return s;
}

LPCWSTR ConvertMultiToWide(LPCSTR m)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, m, -1, NULL, 0);
    LPWSTR s = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, m, -1, s, len);
    return s;
}

bool CheckRTPInstalled()
{
    HKEY k;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Enterbrain\\RGSS3\\RTP"), NULL, KEY_READ, &k) != ERROR_SUCCESS)
    {
        return false;
    }

    BYTE data[MAX_PATH];
    DWORD cb = MAX_PATH;

    if (RegQueryValueEx(k, TEXT("RPGVXAce"), NULL, NULL, data, &cb) != ERROR_SUCCESS)
    {
        return false;
    }

    if (!std::filesystem::exists((LPWSTR)data))
    {
        return false;
    }

    return true;
}

void CheckUpdate(cpr::Response r)
{
    if (r.status_code != 200 || r.error)
    {
        return;
    }

    nlohmann::json j = nlohmann::json::parse(r.text, nullptr, false, true);
    if (j.is_null() || j.is_discarded())
    {
        return;
    }

    if (!j.contains("version"))
    {
        return;
    }

    std::string sVersion = j["version"].get<std::string>();
    if (sVersion.empty())
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
    std::string message = j["message"].get<std::string>();
    if (!message.empty())
    {
        std::string mText = std::format("发现新版本 {0}！\r\n---\r\n{1}\r\n---\r\n是否前往发布页查看详细信息？", sVersion, message);
        text = ConvertMultiToWide(mText.c_str());
    }

    std::wstring pUrl(TEXT("https://www.bilibili.com/opus/1061813870909194291"));
    std::string sUrl = j["url"].get<std::string>();
    if (!sUrl.empty())
    {
        std::wstring wUrl(ConvertMultiToWide(sUrl.c_str()));
        pUrl = wUrl;
    }

    HWND hWnd = GetDesktopWindow();
    if (MessageBox(hWnd, text.c_str(), TEXT("发现更新！"), MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
    {
        ShellExecute(hWnd, TEXT("open"), pUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}
