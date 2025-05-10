#include "crash.h"
#include "dbgdata.h"
#include "utils.h"

#include <dbghelp.h>
#include <psapi.h>

#include <filesystem>
#include <stacktrace>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

[[nodiscard]] [[maybe_unused]] Stacktrace Stacktrace::current(size_t skip, size_t maxDepth)
{
    auto s = std::stacktrace::current(skip + 1, maxDepth);
    Stacktrace res;
    res.entries.reserve(s.size());
    for (auto& entry : s)
    {
        res.entries.emplace_back(entry.native_handle());
    }
    res.hash = std::hash<std::stacktrace>{}(s);
    return res;
}

LONG NTAPI unhandledExceptionFilter(_In_ struct _EXCEPTION_POINTERS* e)
{
    try
    {
        nlohmann::json j{
            {"version", {{"script", VERSION}, {"library", LIBRARY_VERSION}}},
            {"has_3a", std::filesystem::exists("./Game.rgss3a")},
            {"address", std::format("{:#x}", (ULONG64)e->ExceptionRecord->ExceptionAddress)},
            {"code", std::format("{:#x}", e->ExceptionRecord->ExceptionCode)},
            {"flags", std::format("{:#x}", e->ExceptionRecord->ExceptionFlags)},
            {"has_record", e->ExceptionRecord->ExceptionRecord ? true : false} };
        std::vector<std::string> params;
        for (int i = 0; i < e->ExceptionRecord->NumberParameters; ++i)
        {
            params.push_back(std::format("{:#x}", e->ExceptionRecord->ExceptionInformation[i]));
        }

        j["params"] = params;

        std::vector<nlohmann::json> stacktrace;
        STACKFRAME64 sf{};
        sf.AddrPC.Offset = e->ContextRecord->Eip;
        sf.AddrFrame.Offset = e->ContextRecord->Ebp;
        sf.AddrStack.Offset = e->ContextRecord->Esp;
        sf.AddrPC.Mode = AddrModeFlat;
        sf.AddrFrame.Mode = AddrModeFlat;
        sf.AddrStack.Mode = AddrModeFlat;

        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        struct RealStacktrace
        {
            std::vector<decltype(e->ContextRecord->Eip)> addresses;
            size_t hash;
        } realStacktrace;
        for (size_t i = 0; i < ~0ull; ++i)
        {
            SetLastError(0);
            BOOL correct = StackWalk64(
                IMAGE_FILE_MACHINE_I386,
                process,
                thread,
                &sf,
                e->ContextRecord,
                nullptr,
                &SymFunctionTableAccess64,
                &SymGetModuleBase64,
                nullptr);
            if (!correct || !sf.AddrFrame.Offset)
                break;
            realStacktrace.hash += sf.AddrPC.Offset;
            realStacktrace.addresses.push_back(sf.AddrPC.Offset);
        }

        Stacktrace stacktraceData = *reinterpret_cast<Stacktrace*>(&realStacktrace);
        for (int i = 0; i < stacktraceData.size(); ++i)
        {
            StackTraceEntryInfo info = getInfo(stacktraceData[i]);
            nlohmann::json stackj{};

            if (!info.file.empty())
            {
                stackj["file"] = info.file;
            }

            if (!info.name.empty())
            {
                stackj["name"] = info.name;
            }

            if (info.displacement.has_value())
            {
                stackj["displacement"] = std::format("{:#x}", info.displacement.value());
            }

            if (info.line.has_value())
            {
                stackj["line"] = info.line.value();
            }

            stacktrace.push_back(stackj);
        }

        j["stacktrace"] = stacktrace;

        cpr::Url url(URL);
        cpr::Body body(j.dump());
        cpr::AsyncResponse p = cpr::PostAsync(url, body);

        MessageBoxA(nullptr, "Sorry but we were crashed...", "Crashed!!", MB_ICONERROR);

        if (!p.valid())
        {
            MessageBoxA(nullptr, "Is not a vaild post request.", "SUBMIT ERROR", MB_ICONERROR);
        }

        p.wait();
        cpr::Response r = p.get();
        if (r.error)
        {
            MessageBoxA(nullptr, r.error.message.c_str(), "SUBMIT ERROR", MB_ICONERROR);
        }
    }
    catch (std::exception& e)
    {
        MessageBoxA(nullptr, e.what(), "SUBMIT ERROR", MB_ICONERROR);
    }
    return false;
}

LONG NTAPI uncatchableExceptionHandler(_In_ struct _EXCEPTION_POINTERS* e)
{
    static std::atomic_bool onceFlag{ false };
    auto const& code = e->ExceptionRecord->ExceptionCode;
    if (code == STATUS_HEAP_CORRUPTION || code == STATUS_STACK_BUFFER_OVERRUN
        // need to add all can't catch status code
        )
    {
        if (!onceFlag)
        {
            onceFlag = true;
            unhandledExceptionFilter(e);
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
