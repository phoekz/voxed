project_name = "voxed"
ext_dir = "ext"

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

group ("ext")
    project ("imgui")
        kind ("StaticLib")
        files {
            path.join(ext_dir, "imgui-1.50/imgui.cpp"),
            path.join(ext_dir, "imgui-1.50/imgui_draw.cpp"),
            path.join(ext_dir, "imgui-1.50/*.h"),
        }
        filter "configurations:debug"
            defines { "DEBUG" }
        filter "configurations:release"
            defines { "NDEBUG" }
            optimize ("Speed")
    project ("gl3w")
        kind ("StaticLib")
        files {
            path.join(ext_dir, "gl3w/**.c"),
            path.join(ext_dir, "gl3w/**.h"),
        }
        includedirs { path.join(ext_dir, "gl3w/include") }
        filter "configurations:debug"
            defines { "DEBUG" }
        filter "configurations:release"
            defines { "NDEBUG" }
            optimize ("Speed")
group ("")

project (project_name)
    kind ("ConsoleApp")
    warnings ("Extra")
    files {
        "src/**.cpp",
        "src/**.h",
    }
    includedirs {
        path.join(ext_dir, "glm-0.9.8.4/glm"),
        path.join(ext_dir, "imgui-1.50"),
        path.join(ext_dir, "gl3w/include"),
    }

    filter "action:vs*"
        disablewarnings {
            "4201", -- nonstandard extension used: nameless struct/union
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
        includedirs { path.join(ext_dir, "SDL-2.0.4/include") }
        libdirs { path.join(ext_dir, "SDL-2.0.4/bin/win64") }
        defines { "SDL_MAIN_HANDLED" }
        links { "SDL2", "opengl32", "gl3w", "imgui" }
        postbuildcommands { "{COPY} " .. path.join(os.getcwd(), ext_dir, "SDL-2.0.4/bin/win64/SDL2.dll") .. " %{cfg.targetdir}" }
