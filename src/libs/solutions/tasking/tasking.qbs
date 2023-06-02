QtcLibrary {
    name: "Tasking"
    Depends { name: "Qt"; submodules: ["core"] }
    cpp.defines: base.concat("TASKING_LIBRARY")

    files: [
        "barrier.cpp",
        "barrier.h",
        "tasking_global.h",
        "tasktree.cpp",
        "tasktree.h",
    ]
}

