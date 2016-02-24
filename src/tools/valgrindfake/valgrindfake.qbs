import qbs

CppApplication {
    name: "valgrind-fake"
    consoleApplication: true
    destinationDirectory: project.ide_bin_path
    Depends { name: "Qt"; submodules: ["network", "xml"]; }
    cpp.cxxLanguageVersion: "c++11"
    files: [
        "main.cpp",
        "outputgenerator.h", "outputgenerator.cpp"
    ]
}
