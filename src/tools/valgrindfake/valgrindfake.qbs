import qbs

CppApplication {
    name: "valgrind-fake"
    type: "application"
    destinationDirectory: project.ide_bin_path
    Depends { name: "Qt"; submodules: ["network", "xml"]; }
    files: [
        "main.cpp",
        "outputgenerator.h", "outputgenerator.cpp"
    ]
}
