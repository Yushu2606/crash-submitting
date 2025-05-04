add_requires(
    "minhook v1.3.3",
    "nlohmann_json v3.11.3"
)
add_requires("cpr 1.11.1", {configs = {ssl = true}})

target("crash_submitting")
    set_kind("shared")
    set_languages("c++20")
    add_files("src/**.cpp")
    add_includedirs("include")
    add_packages(
        "minhook",
        "nlohmann_json",
        "cpr"
    )
