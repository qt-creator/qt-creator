import qbs

Project {
    name: "ProParser"

    QtcDevHeaders { }

    // Built as required by the QtSupport plugin. Other potential users need to include the sources
    // directly.
    QtcProduct {
        type: ["staticlibrary", "qtc.dev-module"]
        install: qtc.make_dev_package
        installDir: qtc.ide_library_path
        installTags: ["staticlibrary"]

        cpp.defines: base.concat([
            "QMAKE_BUILTIN_PRFS",
            "QMAKE_AS_LIBRARY",
            "PROPARSER_THREAD_SAFE",
            "PROEVALUATOR_THREAD_SAFE",
            "PROEVALUATOR_CUMULATIVE",
            "PROEVALUATOR_SETENV",
        ])

        Depends { name: "Qt.xml" }

        Export {
            Depends { name: "cpp" }
            cpp.defines: base.concat([
                "QMAKE_AS_LIBRARY",
                "PROPARSER_THREAD_SAFE",
                "PROEVALUATOR_THREAD_SAFE",
                "PROEVALUATOR_CUMULATIVE",
                "PROEVALUATOR_SETENV",
            ])
            cpp.includePaths: base.concat([".."])
        }

        files: [
            "ioutils.cpp",
            "ioutils.h",
            "profileevaluator.cpp",
            "profileevaluator.h",
            "proitems.cpp",
            "proitems.h",
            "proparser.qrc",
            "prowriter.cpp",
            "prowriter.h",
            "qmake_global.h",
            "qmakebuiltins.cpp",
            "qmakeevaluator.cpp",
            "qmakeevaluator.h",
            "qmakeevaluator_p.h",
            "qmakeglobals.cpp",
            "qmakeglobals.h",
            "qmakeparser.cpp",
            "qmakeparser.h",
            "qmakevfs.cpp",
            "qmakevfs.h",
        ]
    }
}
