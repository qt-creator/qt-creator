import qbs

CppApplication {
    name: "valgrind-fake"
    consoleApplication: true
    destinationDirectory: qtc.ide_bin_path
    Depends { name: "Qt"; submodules: ["network", "xml"]; }
    Depends { name: "qtc" }
    cpp.cxxLanguageVersion: "c++11"
    files: [
        "main.cpp",
        "outputgenerator.h", "outputgenerator.cpp"
    ]
}
