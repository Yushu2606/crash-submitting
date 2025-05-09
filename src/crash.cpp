#include "crash.h"
#include "dbgdata.h"
#include "utils.h"

#include "dbghelp.h"
#include "psapi.h"

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
        nlohmann::json j;
        std::vector<std::string> params;
        j["exception"] = {
            {"address", std::to_string((ULONG_PTR)e->ExceptionRecord->ExceptionAddress)},
            {"code", std::to_string(e->ExceptionRecord->ExceptionCode)},
            {"flags", std::to_string(e->ExceptionRecord->ExceptionFlags)},
            {"params", params},
            {"has_record", e->ExceptionRecord->ExceptionRecord ? true : false} };
        for (int i = 0; i < e->ExceptionRecord->NumberParameters; ++i)
        {
            params.push_back(std::to_string(e->ExceptionRecord->ExceptionInformation[i]));
        }
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
                NULL,
                &SymFunctionTableAccess64,
                &SymGetModuleBase64,
                NULL);
            if (!correct || !sf.AddrFrame.Offset)
                break;
            realStacktrace.hash += sf.AddrPC.Offset;
            realStacktrace.addresses.push_back(sf.AddrPC.Offset);
        }
        Stacktrace stacktraceData = *reinterpret_cast<Stacktrace*>(&realStacktrace);
        for (int i = 0; i < stacktraceData.size(); ++i)
        {
            StackTraceEntryInfo info = getInfo(stacktraceData[i]);
            nlohmann::json stackj{
                {"file", info.file},
                {"name", info.name},
            };

            if (info.displacement.has_value())
            {
                stackj["displacement"] = std::to_string(info.displacement.value());
            }

            if (info.line.has_value())
            {
                stackj["line"] = std::to_string(info.line.value());
            }

            stacktrace.push_back(stackj);
        }
        j["stacktrace"] = stacktrace;

        cpr::Url url(URL);
        cpr::Body body(j.dump());
        cpr::AsyncResponse p = cpr::PostAsync(url, body);
        if (!p.valid())
        {
            MessageBoxA(NULL, "Is not a vaild post request.", "SUBMIT ERROR", MB_ICONERROR);
        }

        MessageBoxA(NULL, "Sorry but we were crashed...", "Crashed!!", MB_ICONERROR);
        p.wait();
        cpr::Response r = p.get();
        if (r.error)
        {
            MessageBoxA(NULL, r.error.message.c_str(), "SUBMIT ERROR", MB_ICONERROR);
        }
    }
    catch (std::exception& e)
    {
        MessageBoxA(NULL, e.what(), "SUBMIT ERROR", MB_ICONERROR);
    }
    return NULL;
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
