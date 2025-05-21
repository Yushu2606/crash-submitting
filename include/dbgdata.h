// From github.com@LiteLDev/LeviLamina:main@src/ll/api/utils/StacktraceUtils.h

#pragma once

#include <windows.h>

#include <optional>
#include <string>
#include <vector>

class Stacktrace;

class StacktraceEntry
{
    friend Stacktrace;
    void* address{};

public:
    using native_handle_type = void*;

    explicit StacktraceEntry(void* address) : address(address) {}

    void* native_handle() const { return address; }
};
class Stacktrace
{
    std::vector<StacktraceEntry> entries;
    DWORD64 hash{};

public:
    [[nodiscard]] [[maybe_unused]] static Stacktrace current(size_t skip = 0, DWORD64 maxDepth = ~0ull);

    DWORD64 getHash() const { return hash; }

    size_t size() const { return entries.size(); }

    bool empty() const { return entries.empty(); }

    StacktraceEntry const& operator[](size_t index) const { return entries[index]; }
};

struct StackTraceEntryInfo
{
    std::optional<ULONG64> displacement;
    std::string name;
    std::optional<ULONG> line;
    std::string file;
};

[[nodiscard]] [[maybe_unused]] StackTraceEntryInfo getInfo(StacktraceEntry const&);
