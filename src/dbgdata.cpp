// From github.com@LiteLDev/LeviLamina:main@src/ll/api/utils/StacktraceUtils_win.cpp

#include "crash.h"
#include "dbgdata.h"

#include <dbghelp.h>
#include <dbgeng.h>
#include <psapi.h>

#include <filesystem>
#include <stacktrace>

// From github.com@LiteLDev/LeviLamina:main@src/ll/api/utils/SystemUtils.h
template <std::invocable<wchar_t*, size_t, size_t&> Fn>
[[nodiscard]] inline std::optional<std::wstring> adaptFixedSizeToAllocatedResult(Fn&& callback) noexcept
{
    constexpr size_t arraySize = 256;

    wchar_t value[arraySize]{};
    size_t valueLengthNeededWithNull{};

    std::optional<std::wstring> result{ std::in_place };

    if (!std::invoke(std::forward<Fn>(callback), value, arraySize, valueLengthNeededWithNull))
    {
        result.reset();
        return result;
    }
    if (valueLengthNeededWithNull <= arraySize)
    {
        return std::optional<std::wstring>{std::in_place, value, valueLengthNeededWithNull - 1};
    }
    do
    {
        result->resize(valueLengthNeededWithNull - 1);
        if (!std::invoke(std::forward<Fn>(callback), result->data(), result->size() + 1, valueLengthNeededWithNull))
        {
            result.reset();
            return result;
        }
    } while (valueLengthNeededWithNull > result->size() + 1);
    if (valueLengthNeededWithNull <= result->size())
    {
        result->resize(valueLengthNeededWithNull - 1);
    }
    return result;
}

// From github.com@LiteLDev/LeviLamina:main@src/ll/api/utils/SystemUtils_win.cpp
std::optional<std::filesystem::path> getModulePath(HANDLE handle, HANDLE process = nullptr)
{
    return adaptFixedSizeToAllocatedResult(
        [module = static_cast<HMODULE>(handle),
        process](wchar_t* value, size_t valueLength, size_t& valueLengthNeededWithNul) -> bool
        {
            DWORD copiedCount{};
            size_t valueUsedWithNul{};
            bool copyFailed{};
            bool copySucceededWithNoTruncation{};
            if (process != nullptr)
            {
                // GetModuleFileNameExW truncates and provides no error or other indication it has done so.
                // The only way to be sure it didn't truncate is if it didn't need the whole buffer. The
                // count copied to the buffer includes the nul-character as well.
                copiedCount = GetModuleFileNameEx(process, module, value, static_cast<DWORD>(valueLength));
                valueUsedWithNul = static_cast<size_t>(copiedCount) + 1;
                copyFailed = (0 == copiedCount);
                copySucceededWithNoTruncation = !copyFailed && (copiedCount < valueLength - 1);
            }
            else
            {
                // In cases of insufficient buffer, GetModuleFileNameW will return a value equal to
                // lengthWithNull and set the last error to ERROR_INSUFFICIENT_BUFFER. The count returned does
                // not include the nul-character
                copiedCount = GetModuleFileName(module, value, static_cast<DWORD>(valueLength));
                valueUsedWithNul = static_cast<size_t>(copiedCount) + 1;
                copyFailed = (0 == copiedCount);
                copySucceededWithNoTruncation = !copyFailed && (copiedCount < valueLength);
            }
            if (copyFailed)
            {
                return false;
            }
            // When the copy truncated, request another try with more space.
            valueLengthNeededWithNul = copySucceededWithNoTruncation ? valueUsedWithNul : (valueLength * 2);
            return true;
        })
        .transform([](auto&& path)
            { return std::filesystem::path(path); });
}

[[nodiscard]] [[maybe_unused]] Stacktrace Stacktrace::current(size_t skip, DWORD64 maxDepth)
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

static void lockRelease() noexcept;

class [[nodiscard]] DbgEngData
{
public:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    DbgEngData() noexcept { AcquireSRWLockExclusive(&srw); }

    ~DbgEngData() { ReleaseSRWLockExclusive(&srw); }

    DbgEngData(DbgEngData const&) = delete;
    DbgEngData& operator=(DbgEngData const&) = delete;

    void release() noexcept
    {
        // "Phoenix singleton" - destroy and set to null, so that it can be initialized later again

        if (debugClient != nullptr)
        {
            if (attached)
            {
                (void)debugClient->DetachProcesses();
                attached = false;
            }

            debugClient->Release();
            debugClient = nullptr;
        }

        if (debugControl != nullptr)
        {
            debugControl->Release();
            debugControl = nullptr;
        }

        if (debugSymbols != nullptr)
        {
            debugSymbols->Release();
            debugSymbols = nullptr;
        }

        if (dbgEng != nullptr)
        {
            (void)FreeLibrary(dbgEng);
            dbgEng = nullptr;
        }

        initializeAttempted = false;
    }

    [[nodiscard]] bool tryInit() noexcept
    {
        if (!initializeAttempted)
        {
            initializeAttempted = true;

            if (std::atexit(lockRelease) != 0)
            {
                return false;
            }

            dbgEng = LoadLibraryEx(TEXT("dbgeng.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

            if (dbgEng != nullptr)
            {
                const auto debug_create =
                    reinterpret_cast<decltype(&DebugCreate)>(GetProcAddress(dbgEng, "DebugCreate"));

                // Deliberately not calling CoInitialize[Ex]. DbgEng.h API works fine without it.
                // COM initialization may have undesired interference with user's code.
                if (debug_create != nullptr && SUCCEEDED(debug_create(IID_IDebugClient, reinterpret_cast<PVOID*>(&debugClient))) && SUCCEEDED(debugClient->QueryInterface(IID_IDebugSymbols3, reinterpret_cast<PVOID*>(&debugSymbols))) && SUCCEEDED(debugClient->QueryInterface(IID_IDebugControl, reinterpret_cast<PVOID*>(&debugControl))))
                {
                    attached = SUCCEEDED(debugClient->AttachProcess(
                        0,
                        GetCurrentProcessId(),
                        DEBUG_ATTACH_NONINVASIVE | DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND));
                    if (attached)
                    {
                        (void)debugControl->WaitForEvent(0, INFINITE);
                    }
                    (void)debugSymbols->AppendSymbolPathWide(getModulePath(nullptr).value().parent_path().c_str());
                    (void)debugSymbols->RemoveSymbolOptions(
                        SYMOPT_NO_CPP | SYMOPT_LOAD_ANYTHING | SYMOPT_NO_UNQUALIFIED_LOADS | SYMOPT_IGNORE_NT_SYMPATH | SYMOPT_PUBLICS_ONLY | SYMOPT_NO_PUBLICS | SYMOPT_NO_IMAGE_SEARCH);
                    (void)debugSymbols->AddSymbolOptions(
                        SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_EXACT_SYMBOLS | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_AUTO_PUBLICS | SYMOPT_NO_PROMPTS);
                }
            }
        }
        return attached;
    }

    StackTraceEntryInfo getInfo(void const* const address)
    {
        std::optional<ULONG64> displacement = 0;
        std::string name;
        std::optional<ULONG> line = 0;
        std::string file;
        if (ULONG bufSize;
            S_OK == debugSymbols->GetNameByOffset(reinterpret_cast<ULONG64>(address), nullptr, 0, &bufSize, &*displacement))
        {
            std::string buf(bufSize - 1, '\0');
            if (S_OK == debugSymbols->GetNameByOffset(reinterpret_cast<ULONG64>(address), buf.data(), bufSize, nullptr, nullptr))
            {
                name = std::move(buf);
            }
        }
        else
        {
            displacement = std::nullopt;
        }
        if (ULONG bufSize;
            S_OK == debugSymbols->GetLineByOffset(reinterpret_cast<ULONG64>(address), &*line, nullptr, 0, &bufSize, nullptr))
        {
            std::string buf(bufSize - 1, '\0');
            if (S_OK == debugSymbols
                ->GetLineByOffset(reinterpret_cast<ULONG64>(address), nullptr, buf.data(), bufSize, nullptr, nullptr))
            {
                file = std::move(buf);
            }
        }
        else
        {
            line = std::nullopt;
        }
        return { displacement, name, line, file };
    }
    // NOLINTEND(readability-convert-member-functions-to-static)
private:
    inline static SRWLOCK srw = SRWLOCK_INIT;
    inline static IDebugClient* debugClient = nullptr;
    inline static IDebugSymbols3* debugSymbols = nullptr;
    inline static IDebugControl* debugControl = nullptr;
    inline static bool attached = false;
    inline static bool initializeAttempted = false;
    inline static HMODULE dbgEng = nullptr;
};

void lockRelease() noexcept
{
    DbgEngData data;

    data.release();
}

StackTraceEntryInfo getInfo(StacktraceEntry const& entry)
{
    DbgEngData data;

    if (!data.tryInit())
    {
        return {};
    }
    return data.getInfo(entry.native_handle());
}
