import qbs

QtcAutotest {
    name: "Name demangler autotest"

    cpp.enableExceptions: true

    Group {
        name: "Sources from Debugger plugin"
        prefix: project.debuggerDir + "namedemangler/"
        files: ["*.h", "*.cpp"]
    }
    Group {
        name: "Test sources"
        files: "tst_namedemangler.cpp"
    }
    cpp.includePaths: base.concat([project.debuggerDir])
}
