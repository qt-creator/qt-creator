import qbs 1.0

QtcPlugin {
    name: "CompilerExplorer"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Spinner" }
    Depends { name: "TerminalLib" }
    Depends { name: "TextEditor" }
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

        "compilerexplorer.qrc",

        "compilerexploreraspects.cpp",
        "compilerexploreraspects.h",
        "compilerexplorerconstants.h",
        "compilerexplorereditor.cpp",
        "compilerexplorereditor.h",
        "compilerexploreroptions.cpp",
        "compilerexploreroptions.h",
        "compilerexplorerplugin.cpp",
        "compilerexplorersettings.cpp",
        "compilerexplorersettings.h",
        "compilerexplorertr.h",

        "logos/logos.qrc"
    ]
}
