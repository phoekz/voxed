project_name = "voxed"
ext_path = "ext/"

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

    filter "action:vs*"
        defines { "_CRT_SECURE_NO_WARNINGS" }

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

    filter "system:macosx"
        includedirs { "/Library/Frameworks/SDL2.framework/Headers" }
        links { "Cocoa.framework" }
        linkoptions { "-F/Library/Frameworks -framework SDL2" }

    filter "system:windows"
        includedirs { ext_path.."SDL-2.0.4/include" }
        libdirs { ext_path.."SDL-2.0.4/bin/win64" }
        links { "SDL2", "SDL2main", "opengl32" }
        postbuildcommands { "{COPY} ".. ext_path.."SDL-2.0.4/bin/win64/SDL2.dll %{cfg.targetdir}" }
