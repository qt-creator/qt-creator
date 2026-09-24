import qbs

QtcAutotest {
    name: "Debugger JDWP autotest"
    Depends { name: "Debugger" }
    Depends { name: "Utils" }
    Depends { name: "Qt.network" }
    Group {
        name: "Sources from Debugger plugin"
        prefix: project.debuggerDir
        files: ["debuggerprotocol.h", "debuggerprotocol.cpp"]
    }
    Group {
        name: "Test sources"
        files: "tst_jdwp.cpp"
    }
    Group {
        name: "Inferior sources"
        prefix: "jdwp/org/qtproject/jdwptest/"
        files: ["Helper.java", "Inferior.java"]
    }
    cpp.defines: base.concat(['JDWP_TEST_SOURCE_DIR="' + path + '/jdwp"'])
    cpp.includePaths: base.concat([project.debuggerDir])
}
