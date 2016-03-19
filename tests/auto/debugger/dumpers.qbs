import qbs

QtcAutotest {
    name: "Debugger dumpers autotest"
    Depends { name: "Utils" }
    Depends { name: "Qt.network" } // For QHostAddress
    Depends { name: "Qt.widgets" }
    Group {
        name: "Sources from Debugger plugin"
        prefix: project.debuggerDir
        files: [
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
        'CDBEXT_PATH="' + project.buildDirectory + '\\\\lib"',
        'DUMPERDIR="' + path + '/../../../share/qtcreator/debugger"',
    ])
    cpp.includePaths: base.concat([project.debuggerDir])
}
