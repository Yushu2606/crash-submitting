#include "events.h"
#include "crash.h"
#include "utils.h"

#include <shellapi.h>

#include <filesystem>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

void on_start(const char* url_p, const char* version_p, bool is_test_version)
{
    URL = std::string(url_p);
    VERSION = std::string(version_p);
    IS_TEST_VERSION = is_test_version;

    AddVectoredExceptionHandler(1, uncatchableExceptionHandler);
    SetUnhandledExceptionFilter(unhandledExceptionFilter);

    cpr::Url url(URL + "/api/v1/version");
    cpr::GetCallback(CheckUpdate, url);
}

void on_error(const char* typename_p, const char* message_p, const char* stack_p)
{
    auto hWnd = GetDesktopWindow();
    if (typename_p == "Errno::ENOENT" || !CheckRTPInstalled())
    {
        if (MessageBox(hWnd, TEXT("您好像没有正确安装RPG Maker VX Ace Run Time Package，是否前往官网进行下载？"), TEXT("彼阳的汉化组"), MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
        {
            ShellExecute(hWnd, TEXT("open"), TEXT("https://dl.komodo.jp/rpgmakerweb/run-time-packages/RPGVXAce_RTP.zip"), nullptr, nullptr, SW_SHOWNORMAL);
            return;
        }
    }

    ShowWindow(hWnd, SW_HIDE);

    auto stack = Slice(stack_p, "\r\n");
    if (stack.size() <= 1)
    {
        stack = Slice(stack_p, "\n");
    }

    std::vector<nlohmann::json> jStack;
    jStack.reserve(stack.size());
    for (auto& trace : stack)
    {
        auto jTrace = nlohmann::json::parse(trace);
        for (auto& [k, v] : jTrace.items())
        {
            if (v.get<std::string>().empty())
            {
                jTrace.erase(k);
            }
        }

        jStack.push_back(std::move(jTrace));
    }

    std::string type(typename_p);
    nlohmann::json j{
        {"_version", {{"game", std::move(VERSION)}, {"module", LIBRARY_VERSION}}},
        {"packaged", std::filesystem::exists("./Game.rgss3a")},
        {"type", std::move(type)},
        {"stack", std::move(jStack)} };
    std::string message(message_p);
    if (!message.empty())
    {
        j["message"] = std::move(message);
    }

    cpr::Url url(URL);
    cpr::Body body(j.dump());
    cpr::Post(url, body);
}
