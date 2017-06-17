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

    filter "configurations:debug"
    defines { "DEBUG" }

    filter "configurations:release"
        defines { "NDEBUG" }
        optimize ("Speed")

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
    project ("gl3w")
        kind ("StaticLib")
        files {
            path.join(ext_dir, "gl3w/**.c"),
            path.join(ext_dir, "gl3w/**.h"),
        }
        includedirs { path.join(ext_dir, "gl3w/include") }

group ("")

project (project_name)
    kind ("ConsoleApp")
    warnings ("Extra")

    files {
        "src/integrations/imgui/**",
        "src/platform/**",
        "src/common/**.h",
        "src/main.cpp"
    }

    includedirs {
        path.join(ext_dir, "glm-0.9.8.4/glm"),
        path.join(ext_dir, "imgui-1.50"),
        path.join(ext_dir, "gl3w/include"),
        path.join("src"),
    }

    filter "action:vs*"
        disablewarnings {
            "4201", -- nonstandard extension used: nameless struct/union
        }

    filter "action:gmake"
        buildoptions { "-std=c++14" }

    filter "system:macosx"
        files {
            "src/integrations/mtl/**.mm",
        }
        includedirs { "/usr/local/Cellar/sdl2/2.0.5/include/SDL2" }
        libdirs { "/usr/local/Cellar/sdl2/2.0.5/lib" }
        links { "SDL2" }

    filter "system:windows"
        files {
            "src/integrations/gl/**",
        }
        includedirs { path.join(ext_dir, "SDL-2.0.4/include") }
        libdirs { path.join(ext_dir, "SDL-2.0.4/bin/win64") }
        defines { "SDL_MAIN_HANDLED" }
        links { "SDL2", "opengl32", "gl3w", "imgui" }
        postbuildcommands { "{COPY} " .. path.join(os.getcwd(), ext_dir, "SDL-2.0.4/bin/win64/SDL2.dll") .. " %{cfg.targetdir}" }
