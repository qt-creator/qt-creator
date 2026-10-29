import qbs

QtcAutotest {
    name: "Debugger JDWP autotest"
    Depends { name: "Debugger" }
    Depends { name: "Utils" }
    Depends { name: "Qt.network" }
    Group {
        name: "Test sources"
        files: "tst_jdwp.cpp"
    }
    Group {
        name: "Inferior sources"
        prefix: "jdwp/org/qtproject/jdwptest/"
        files: ["Helper.java", "Inferior.java", "other/Helper.java"]
    }
    cpp.defines: base.concat(['JDWP_TEST_SOURCE_DIR="' + path + '/jdwp"'])
}
