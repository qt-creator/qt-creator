import qbs

QtcAutotest {
    name: "ProParser autotest"
    Depends { name: "Utils" }
    Group {
        name: "Sources from ProParser"
        id: proParserGroup
        prefix: project.sharedSourcesDir + "/proparser/"
        files: [
            "ioutils.cpp", "ioutils.h",
            "proitems.cpp", "proitems.h",
            "qmake_global.h",
            "qmakebuiltins.cpp",
            "qmakeevaluator.cpp", "qmakeevaluator.h", "qmakeevaluator_p.h",
            "qmakeglobals.cpp", "qmakeglobals.h",
            "qmakeparser.cpp", "qmakeparser.h",
            "qmakevfs.cpp", "qmakevfs.h",
            "registry.cpp", "registry_p.h",
        ]
    }
    Group {
        name: "Test sources"
        files: "tst_proparser.cpp"
    }
    cpp.includePaths: base.concat([proParserGroup.prefix])
    cpp.defines: base.concat("QT_USE_QSTRINGBUILDER")
    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: "advapi32"
    }
}
