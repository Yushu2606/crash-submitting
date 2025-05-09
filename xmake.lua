add_rules("mode.debug", "mode.release")

add_requires(
    "minhook v1.3.3",
    "nlohmann_json v3.11.3"
)
add_requires("cpr 1.11.1", { configs = { ssl = true } })

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("rgss_telemetry")
    add_cxflags("/utf-8")
    add_defines(
        "DBGHELP_TRANSLATE_TCHAR",
        "NOMINMAX", -- To avoid conflicts with std::min and std::max.
        "UNICODE", -- To enable Unicode support in Windows API.
        "WIN32_LEAN_AND_MEAN"
    )
    set_kind("shared")
    set_languages("c++latest")
    add_files("src/**.cpp")
    add_includedirs("include")
    add_packages(
        "minhook",
        "nlohmann_json",
        "cpr"
    )
    add_syslinks("dbghelp")
