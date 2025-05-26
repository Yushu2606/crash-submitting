#include "crash.h"
#include "dbgdata.h"
#include "utils.h"

#include <dbghelp.h>

#include <filesystem>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

LONG WINAPI unhandledExceptionFilter(_In_ struct _EXCEPTION_POINTERS* ExceptionInfo)
{
    wchar_t* msg = nullptr;
    DWORD size{};
    auto hWnd = GetDesktopWindow();

    try
    {
        static auto langId = []
            {
                DWORD res{};
                if (GetLocaleInfoEx(
                    LOCALE_NAME_SYSTEM_DEFAULT,
                    LOCALE_SNAME | LOCALE_RETURN_NUMBER,
                    reinterpret_cast<LPWSTR>(&res),
                    sizeof(res) / sizeof(wchar_t)) == 0)
                {
                    res = 0;
                }
                return res;
            }();
        static auto nt = GetModuleHandle(TEXT("ntdll"));

        size = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
            nt,
            ExceptionInfo->ExceptionRecord->ExceptionCode,
            langId,
            reinterpret_cast<LPWSTR>(&msg),
            NULL,
            nullptr);

        nlohmann::json j{
            {"_version", {{"game", VERSION}, {"module", LIBRARY_VERSION}}},
            {"packaged", std::filesystem::exists("./Game.rgss3a")},
            {"address", std::format("{:#x}", reinterpret_cast<size_t>(ExceptionInfo->ExceptionRecord->ExceptionAddress))},
            {"code", std::format("{:#x}", ExceptionInfo->ExceptionRecord->ExceptionCode)},
            {"flags", std::format("{:#x}", ExceptionInfo->ExceptionRecord->ExceptionFlags)},
            {"has_record", ExceptionInfo->ExceptionRecord->ExceptionRecord != nullptr} };
        if (msg != nullptr && size > 0)
        {
            j["message"] = std::move(msg);
        }

        std::vector<std::string> params;
        params.reserve(ExceptionInfo->ExceptionRecord->NumberParameters);
        for (size_t i{}; i < ExceptionInfo->ExceptionRecord->NumberParameters; ++i)
        {
            params.push_back(std::format("{:#x}", ExceptionInfo->ExceptionRecord->ExceptionInformation[i]));
        }

        j["params"] = std::move(params);

        std::vector<nlohmann::json> stack;
        STACKFRAME64 sf{};
        sf.AddrPC.Offset = ExceptionInfo->ContextRecord->Eip;
        sf.AddrFrame.Offset = ExceptionInfo->ContextRecord->Ebp;
        sf.AddrStack.Offset = ExceptionInfo->ContextRecord->Esp;
        sf.AddrPC.Mode = AddrModeFlat;
        sf.AddrFrame.Mode = AddrModeFlat;
        sf.AddrStack.Mode = AddrModeFlat;

        auto process = GetCurrentProcess();
        auto thread = GetCurrentThread();

        struct RealStacktrace
        {
            std::vector<DWORD64> addresses;
            DWORD64 hash{};
        } realStacktrace;

        for (DWORD64 i{}; i < ~0ull; ++i)
        {
            SetLastError(0);
            auto correct = StackWalk64(
                IMAGE_FILE_MACHINE_I386,
                process,
                thread,
                &sf,
                ExceptionInfo->ContextRecord,
                nullptr,
                &SymFunctionTableAccess64,
                &SymGetModuleBase64,
                nullptr);
            if (!correct || !sf.AddrFrame.Offset)
            {
                break;
            }

            realStacktrace.hash += sf.AddrPC.Offset;
            realStacktrace.addresses.push_back(sf.AddrPC.Offset);
        }

        auto& stacktrace = *reinterpret_cast<Stacktrace*>(&realStacktrace);
        stack.reserve(stacktrace.size());
        for (size_t i{}; i < stacktrace.size(); ++i)
        {
            auto info = getInfo(stacktrace[i]);
            nlohmann::json stackj;

            if (!info.file.empty())
            {
                stackj["file"] = std::move(info.file);
            }

            if (!info.name.empty())
            {
                stackj["name"] = std::move(info.name);
            }

            if (info.displacement.has_value())
            {
                stackj["displacement"] = std::format("{:#x}", info.displacement.value());
            }

            if (info.line.has_value())
            {
                stackj["line"] = info.line.value();
            }

            stack.push_back(std::move(stackj));
        }

        j["stack"] = std::move(stack);

        cpr::Url url(URL);
        cpr::Body body(j.dump());
        auto r = cpr::Post(url, body);
        if (r.error || r.status_code != 200)
        {
            throw std::exception();
        }

        MessageBoxW(hWnd, TEXT("非常抱歉！这边出了点问题TT"), TITLE, MB_ICONERROR);
    }
    catch (...)
    {
        std::wstring text;
        if (msg != nullptr && size > 0)
        {
            text = std::format(TEXT("非常抱歉！这边出了点问题TT\r\n\r\n{}"), msg);
        }
        else
        {
            text = std::format(TEXT("非常抱歉！这边出了点问题TT（{:#x}）"), ExceptionInfo->ExceptionRecord->ExceptionCode);
        }

        MessageBoxW(hWnd, text.c_str(), TITLE, MB_ICONERROR);
    }

    return true;
}

LONG NTAPI uncatchableExceptionHandler(struct _EXCEPTION_POINTERS* ExceptionInfo)
{
    try
    {
        static std::atomic_bool onceFlag{ false };
        auto const& code = ExceptionInfo->ExceptionRecord->ExceptionCode;
        if (code == STATUS_HEAP_CORRUPTION || code == STATUS_STACK_BUFFER_OVERRUN
            // need to add all can't catch status code
            )
        {
            if (!onceFlag)
            {
                onceFlag = true;
                unhandledExceptionFilter(ExceptionInfo);
            }
        }
    }
    catch (...)
    {
    }

    return EXCEPTION_CONTINUE_SEARCH;
}
