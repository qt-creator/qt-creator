import qbs

CppApplication {
    name: "Gui auto test"
    targetName: "tst_gui"

    Depends { name: "Qt"; submodules: [ "gui", "widgets", "test" ] }
    Depends { name: "cpp" }

    files: [ "tst_guitest.cpp" ]

    cpp.defines: base.concat("SRCDIR=" + path)
}
