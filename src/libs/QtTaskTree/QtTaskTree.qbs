QtcLibrary {
    name: "QtTaskTree"

    Depends { name: "Qt"; submodules: ["concurrent", "core-private", "network"] }

    cpp.defines: base.concat("QTTASKTREE_LIBRARY")
    cpp.includePaths: ".."

    files: [
        "qbarriertask.cpp",
        "qbarriertask.h",
        "qconcurrentcalltask.cpp",
        "qconcurrentcalltask.h",
        "qconditional.cpp",
        "qconditional.h",
        "qnetworkreplywrappertask.cpp",
        "qnetworkreplywrappertask.h",
        "qprocesstask.cpp",
        "qprocesstask.h",
        "qtasktree.cpp",
        "qtasktree.h",
        "qtasktreerunner.cpp",
        "qtasktreerunner.h",
        "qtcpsocketwrappertask.cpp",
        "qtcpsocketwrappertask.h",
        "qttasktreeglobal.h",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [
            exportingProduct.sourceDirectory + "/..",
            exportingProduct.sourceDirectory + "/../.."
        ]
    }
}

