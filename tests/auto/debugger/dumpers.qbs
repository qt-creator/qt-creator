import qbs.FileInfo

QtcAutotest {
    name: "Debugger dumpers autotest"
    Depends { name: "Utils" }
    Depends { name: "Qt.network" } // For QHostAddress
    Depends { name: "Qt.widgets" }
    Group {
        name: "Sources from Debugger plugin"
        prefix: project.debuggerDir
        files: [
            "debuggertr.h",
            "debuggerprotocol.h", "debuggerprotocol.cpp",
            "simplifytype.h", "simplifytype.cpp",
            "watchdata.h", "watchdata.cpp",
            "watchutils.h", "watchutils.cpp"
        ]
    }

    Group {
        name: "Test sources"
        files: [
            "tst_dumpers.cpp"
        ]
    }

    cpp.defines: base.concat([
        'CDBEXT_PATH="' + FileInfo.fromNativeSeparators(FileInfo.joinPaths(qbs.installRoot,
                qbs.installPrefix, qtc.libDirName)) + '"',
        'DUMPERDIR="' + path + '/../../../share/qtcreator/debugger"',
        'DEFAULT_QMAKE_BINARY="qmake"'
    ])
    cpp.includePaths: base.concat([project.debuggerDir])
}
