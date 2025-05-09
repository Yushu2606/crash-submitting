#pragma once

#include "windows.h"
#include <optional>
#include <string>
#include <vector>

struct StackTraceEntryInfo {
    std::optional<ULONG64> displacement;
    std::string           name;
    std::optional<ULONG>  line;
    std::string           file;
};
class Stacktrace;
class StacktraceEntry {
    friend Stacktrace;
    void* address{};

public:
    using native_handle_type = void*;

    explicit StacktraceEntry(void* address) : address(address) {}

    void* native_handle() const { return address; }
};
class Stacktrace {
    std::vector<StacktraceEntry> entries;
    unsigned long long                       hash;

public:
    [[nodiscard]] [[maybe_unused]] static Stacktrace current(size_t skip = 0, size_t maxDepth = ~0ull);

    unsigned long long getHash() const { return hash; }

    size_t size() const { return entries.size(); }

    bool empty() const { return entries.empty(); }

    StacktraceEntry const& operator[](size_t index) const { return entries[index]; }
};

StackTraceEntryInfo getInfo(StacktraceEntry const&);
