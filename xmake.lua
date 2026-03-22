add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

set_project("planka-mcp")
set_version("26.03.22")
set_languages("cxx20")

-- 引入 wfrest (基于静态链接，将自动附带引入 workflow)
add_requires("workflow", {configs = {shared = false}})
add_requires("wfrest", {configs = {shared = false}})
add_requires("openssl")

-- 将无构建系统的第三方库打包方案分离出主文件
includes("packages/coke/xmake.lua")
includes("packages/maplog/xmake.lua")

add_requires("coke")
add_requires("maplog")

-- 主程序构建目标
target("planka-mcp")
    set_kind("binary")
    add_configfiles("src/config.h.in")
    add_includedirs("src")
    add_includedirs("$(builddir)")
    add_files("src/**.cpp")
    add_packages("workflow", "wfrest", "coke", "openssl", "maplog")

-- 测试目标 (TDD 约束入口)
target("test_planka_mcp")
    set_kind("binary")
    add_configfiles("src/config.h.in")
    add_includedirs("src", "tests")
    add_includedirs("$(builddir)")
    add_files("src/**.cpp|main.cpp")
    add_files("tests/**.cpp")
    add_packages("workflow", "wfrest", "coke", "openssl", "maplog")
