add_rules("mode.debug", "mode.release")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

-- BetterF3 is a client-side mod. The server target is not supported.
option("target_type")
    set_default("client")
    set_showmenu(true)
    set_values("client")
option_end()

add_requires("levilamina 26.51.*", {configs = {target_type = get_config("target_type")}})
add_requires("levibuildscript")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("BetterF3")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker", {modVersion = "0.1.0"})
    if is_plat("windows") then
        add_defines("NOMINMAX", "UNICODE", "_HAS_CXX23=1")
        set_exceptions("none") -- To avoid conflicts with /EHa.
        add_cxflags("/EHa", "/utf-8", "/W4", "/w44265", "/w44289", "/w44296", "/w45263", "/w44738", "/w45204")
        add_cxflags(
            "/EHs",
            "-Wno-microsoft-cast",
            "-Wno-invalid-offsetof",
            "-Wno-c++2b-extensions",
            "-Wno-microsoft-include",
            "-Wno-overloaded-virtual",
            "-Wno-ignored-qualifiers",
            "-Wno-missing-field-initializers",
            "-Wno-potentially-evaluated-expression",
            "-Wno-pragma-system-header-outside-header",
            {tools = {"clang_cl"}}
        )
        set_toolchains("clang-cl")
    end
    add_packages("levilamina")
    -- GPU name / utilization, process memory and window messages for the System module
    add_syslinks("dxgi", "pdh", "psapi", "advapi32", "user32")
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")
    add_headerfiles("src/**.h")
    add_files("src/**.cpp")
    add_includedirs("src")
    -- Copy the language files (en_us / ja_jp) next to the built mod
    after_build(function (target)
        local dest = path.join(os.projectdir(), "bin", target:name(), "assets")
        os.mkdir(dest)
        os.cp(path.join(os.projectdir(), "assets", "*"), dest)
    end)
