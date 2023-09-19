QtcManualTest {
    name: "Manual ProParser test"
    Depends { name: "Qt.core" }

    cpp.includePaths: base.concat(["../../../src/shared/proparser/",
                                   "../../../src/libs/"])

    cpp.defines: ["QMAKE_BUILTIN_PRFS",
                  "QT_NO_CAST_TO_ASCII",
                  "QT_RESTRICTED_CAST_FROM_ASCII",
                  "QT_USE_QSTRINGBUILDER",
                  "PROEVALUATOR_FULL",
                  "PROEVALUATOR_CUMULATIVE",
                  "PROEVALUATOR_INIT_PROPS"]

    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: "advapi32"
    }

    files: "main.cpp"

    Group {
        name: "ProParser files"
        prefix: "../../../src/shared/proparser/"

        files: [
            "ioutils.cpp",
            "ioutils.h",
            "profileevaluator.cpp",
            "profileevaluator.h",
            "proitems.cpp",
            "proitems.h",
            "proparser.qrc",
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
            "registry.cpp",
            "registry_p.h",
        ]
    }

    Group {
        name: "Porting Helper"
        prefix: "../../../src/libs/utils/"
    }
}
