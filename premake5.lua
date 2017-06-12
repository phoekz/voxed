project_name = "voxed"

workspace (project_name)
    configurations { "debug", "release" }
    language ("C++")
    architecture ("x86_64")
    debugdir (".")
    floatingpoint ("Fast")
    fpu ("Hardware")
    location ("build/" .. os.get())
    symbols ("On")
    targetdir ("bin/%{cfg.system}/%{cfg.buildcfg}")

project (project_name)
    kind ("ConsoleApp")
    warnings ("Extra")
    files {
        "src/**.cpp",
        "src/**.h",
    }
    filter "configurations:debug"
        defines { "DEBUG" }
    filter "configurations:release"
        defines { "NDEBUG" }
        optimize ("Speed")
