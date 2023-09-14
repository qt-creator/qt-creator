QtcLibrary {
    name: "Tasking"

    Depends { name: "Qt"; submodules: ["concurrent", "network"] }

    cpp.defines: base.concat("TASKING_LIBRARY")

    files: [
        "barrier.cpp",
        "barrier.h",
        "concurrentcall.h",
        "networkquery.cpp",
        "networkquery.h",
        "tasking_global.h",
        "tasktree.cpp",
        "tasktree.h",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: ["..", "../.."]
    }
}

