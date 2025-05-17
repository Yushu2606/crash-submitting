#include "main.h"
#include "crash.h"
#include "utils.h"

#include <shellapi.h>

#include <filesystem>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

void on_start(LPCSTR url_p, LPCSTR version_p, bool is_test_version)
{
    URL = std::string(url_p);
    VERSION = std::string(version_p);

    AddVectoredExceptionHandler(1, uncatchableExceptionHandler);
    SetUnhandledExceptionFilter(unhandledExceptionFilter);

    cpr::Url url(URL + "/api/v1/version");
    cpr::GetCallback(CheckUpdate, url);
}

void on_error(LPCSTR typename_p, LPCSTR message_p, LPCSTR stack_p)
{
    std::string type(typename_p);
    HWND hWnd = GetDesktopWindow();

    if (type == "Errno::ENOENT" || !CheckRTPInstalled())
    {
        if (MessageBox(hWnd, TEXT("您好像没有正确安装RPG Maker VX Ace Run Time Package，是否前往官网进行下载？"), TEXT("彼阳的汉化组"), MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
        {
            ShellExecute(hWnd, TEXT("open"), TEXT("https://dl.komodo.jp/rpgmakerweb/run-time-packages/RPGVXAce_RTP.zip"), NULL, NULL, SW_SHOWNORMAL);
            return;
        }
    }

    ShowWindow(hWnd, SW_HIDE);
    std::string stack(stack_p);
    std::size_t previous = 0;
    std::size_t current = stack.find("\r\n");
    std::vector<std::string> trace;
    while (current != std::string::npos)
    {
        if (current > previous)
        {
            trace.push_back(stack.substr(previous, current - previous));
        }
        previous = current + 1;
        current = stack.find("\r\n", previous);
    }

    if (previous != stack.size())
    {
        trace.push_back(stack.substr(previous));
    }

    nlohmann::json j{
        {"_version", {{"game", VERSION}, {"module", LIBRARY_VERSION}}},
        {"packaged", std::filesystem::exists("./Game.rgss3a")},
        {"type", type},
        {"stack", trace} };
    std::string message(message_p);
    if (!message.empty())
    {
        j["message"] = message;
    }

    cpr::Url url(URL);
    cpr::Body body(j.dump());
    cpr::Post(url, body);
}
