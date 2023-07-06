import qbs 1.0

QtcPlugin {
    name: "CompilerExplorer"

    Depends { name: "Core" }
    Depends { name: "Qt"; submodules: ["widgets", "network"] }

    files: [
        "api/compile.cpp",
        "api/compile.h",
        "api/compiler.cpp",
        "api/compiler.h",
        "api/compilerexplorerapi.h",
        "api/config.h",
        "api/language.cpp",
        "api/language.h",
        "api/library.cpp",
        "api/library.h",
        "api/request.h",

        "compilerexploreraspects.cpp",
        "compilerexploreraspects.h",
        "compilerexplorerconstants.h",
        "compilerexplorereditor.cpp",
        "compilerexplorereditor.h",
        "compilerexploreroptions.cpp",
        "compilerexploreroptions.h",
        "compilerexplorerplugin.cpp",
        "compilerexplorerplugin.h",
        "compilerexplorersettings.cpp",
        "compilerexplorersettings.h",
        "compilerexplorertr.h",

        "logos/logos.qrc"
    ]
}
