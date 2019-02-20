import qbs

QtcAutotest {
    name: "ProFileWriter autotest"
    Depends { name: "Qt.xml" }
    Depends { name: "Utils"; cpp.link: false }
    Group {
        name: "Sources from ProParser"
        id: proParserGroup
        prefix: project.sharedSourcesDir + "/proparser/"
        files: [
            "ioutils.h", "ioutils.cpp",
            "profileevaluator.h", "profileevaluator.cpp",
            "proitems.h", "proitems.cpp",
            "prowriter.h", "prowriter.cpp",
            "qmake_global.h",
            "qmakebuiltins.cpp",
            "qmakeevaluator.h", "qmakeevaluator_p.h", "qmakeevaluator.cpp",
            "qmakeglobals.h", "qmakeglobals.cpp",
            "qmakeparser.h", "qmakeparser.cpp",
            "qmakevfs.h", "qmakevfs.cpp",
            "registry_p.h", "registry.cpp",
        ]
    }
    Group {
        name: "Test sources"
        files: "tst_profilewriter.cpp"
    }
    cpp.includePaths: base.concat([proParserGroup.prefix])
    cpp.defines: base.concat("QT_USE_FAST_OPERATOR_PLUS")
    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: "advapi32"
    }
}
