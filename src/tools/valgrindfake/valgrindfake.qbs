import qbs

CppApplication {
    name: "valgrind-fake"
    type: "application"
    condition: qbs.targetOS.contains("unix")
    destinationDirectory: project.ide_bin_path
    Depends { name: "Qt"; submodules: ["network", "xml"]; }
    files: [
        "main.cpp",
        "outputgenerator.h", "outputgenerator.cpp"
    ]
}
