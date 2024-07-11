QtcLibrary {
    name: "Tasking"

    Depends { name: "Qt"; submodules: ["concurrent", "network"] }

    cpp.defines: base.concat("TASKING_LIBRARY")

    files: [
        "barrier.cpp",
        "barrier.h",
        "concurrentcall.h",
        "conditional.cpp",
        "conditional.h",
        "networkquery.cpp",
        "networkquery.h",
        "qprocesstask.cpp",
        "qprocesstask.h",
        "tasking_global.h",
        "tasktree.cpp",
        "tasktree.h",
        "tasktreerunner.cpp",
        "tasktreerunner.h",
        "tcpsocket.cpp",
        "tcpsocket.h",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [
            exportingProduct.sourceDirectory + "/..",
            exportingProduct.sourceDirectory + "/../.."
        ]
    }
}

