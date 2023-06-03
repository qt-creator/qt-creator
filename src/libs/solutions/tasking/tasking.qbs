QtcLibrary {
    name: "Tasking"
    Depends { name: "Qt"; submodules: ["concurrent", "core"] }
    cpp.defines: base.concat("TASKING_LIBRARY")

    files: [
        "barrier.cpp",
        "barrier.h",
        "concurrentcall.h",
        "tasking_global.h",
        "tasktree.cpp",
        "tasktree.h",
    ]
}

