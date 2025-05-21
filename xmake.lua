add_rules("mode.debug", "mode.release")

add_requires("nlohmann_json v3.12.0")
add_requires("cpr 1.11.1", { configs = { ssl = true } })

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("rgss_telemetry")
    set_toolchains("llvm")
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
    add_packages("cpr", "nlohmann_json")
    add_syslinks("dbghelp", "shell32")
