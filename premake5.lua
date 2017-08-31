project_name = "voxed"
ext_dir = "ext"

imgui_dir = "imgui-1.51"

workspace (project_name)
    configurations { "debug", "release" }
    language ("C++")
    architecture ("x86_64")
    debugdir ("bin/%{cfg.system}/%{cfg.buildcfg}")
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
            path.join(ext_dir, imgui_dir, "imgui.cpp"),
            path.join(ext_dir, imgui_dir, "imgui_draw.cpp"),
            path.join(ext_dir, imgui_dir, "*.h"),
        }

group ("")

project (project_name)
    kind ("ConsoleApp")
    warnings ("Extra")

    files {
        "src/integrations/imgui/**",
        "src/platform/**",
        "src/common/**",
        "src/editor/**",
        "src/main.cpp",
        "src/voxed.*"
    }

    includedirs {
        path.join(ext_dir, "glm-0.9.8.4/glm"),
        path.join(ext_dir, imgui_dir),
        path.join("src"),
    }

    links { "SDL2", "imgui" }

    filter "action:vs*"
        disablewarnings {
            "4201", -- nonstandard extension used: nameless struct/union
        }

    filter "action:gmake"
        buildoptions { "-std=c++14" }

    filter "system:macosx"
        files {
            "src/integrations/mtl/**",
            "src/shaders/**.metal",
        }
        includedirs { "/usr/local/Cellar/sdl2/2.0.5/include/SDL2" }
        libdirs { "/usr/local/Cellar/sdl2/2.0.5/lib" }
        links {
            "Foundation.framework",
            "Cocoa.framework",
            "Metal.framework",
            "Quartz.framework",
        }
        filter "files:**.metal"
            buildmessage ("Compiling %{file.relpath}")
            buildcommands {
                "exec "
                .. path.join(os.getcwd(), "scripts/build_metal_shaders.sh")
                .. " "
                .. "%{file.abspath}"
                .. " "
                .. "%{cfg.targetdir}/shaders/mtl/%{file.basename}.metallib"
            }
            buildoutputs { "%{cfg.targetdir}/shaders/mtl/%{file.basename}.metallib" }

    filter "system:windows"
        files {
            "src/integrations/gl/**",
            "src/shaders/**.glsl",
        }
        includedirs {
            path.join(ext_dir, "SDL-2.0.4/include"),
        }

        libdirs { path.join(ext_dir, "SDL-2.0.4/bin/win64") }
        defines { "SDL_MAIN_HANDLED" }
        links { "opengl32" }
        postbuildcommands { "{COPY} " .. path.join(os.getcwd(), ext_dir, "SDL-2.0.4/bin/win64/SDL2.dll") .. " %{cfg.targetdir}" }

        filter "files:**.glsl"
            buildcommands { "{COPY} %{file.abspath} %{cfg.targetdir}/shaders/gl" }
            buildoutputs { "%{cfg.targetdir}/shaders/gl/%{file.name}" }
