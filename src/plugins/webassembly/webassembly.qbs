import qbs 1.0

QtcPlugin {
    name: "WebAssembly"

    Depends { name: "Qt.core" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }

    files: [
        "webassembly.qrc",
        "webassembly_global.h",
        "webassemblyconstants.h",
        "webassemblydevice.cpp",
        "webassemblydevice.h",
        "webassemblyemsdk.cpp",
        "webassemblyemsdk.h",
        "webassemblyplugin.cpp",
        "webassemblyplugin.h",
        "webassemblyqtversion.cpp",
        "webassemblyqtversion.h",
        "webassemblyrunconfiguration.cpp",
        "webassemblyrunconfiguration.h",
        "webassemblysettings.cpp",
        "webassemblysettings.h",
        "webassemblytoolchain.cpp",
        "webassemblytoolchain.h",
        "webassemblytr.h",
    ]

    QtcTestFiles {
        files: [
            "webassembly_test.cpp",
            "webassembly_test.h",
        ]
    }
}
