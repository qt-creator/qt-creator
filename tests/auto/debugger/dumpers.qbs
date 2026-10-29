import qbs.FileInfo

QtcAutotest {
    name: "Debugger dumpers autotest"
    Depends { name: "Debugger" }
    Depends { name: "Utils" }
    Depends { name: "Qt.network" } // For QHostAddress
    Depends { name: "Qt.widgets" }

    files: "tst_dumpers.cpp"

    cpp.defines: base.concat([
        'CDBEXT_PATH="' + FileInfo.fromNativeSeparators(FileInfo.joinPaths(qbs.installRoot,
                qbs.installPrefix, qtc.libDirName)) + '"',
        'DUMPERDIR="' + path + '/../../../share/qtcreator/debugger"',
        'DEFAULT_QMAKE_BINARY="qmake"'
    ])
}
