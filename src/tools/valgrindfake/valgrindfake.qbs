import qbs
import qbs.Utilities

QtcTool {
    name: "valgrind-fake"
    consoleApplication: true
    destinationDirectory: qtc.ide_bin_path
    install: false
    sanitizable: false
    Depends { name: "Qt"; submodules: ["network", "xml"]; }

    files: [
        "main.cpp",
        "outputgenerator.h", "outputgenerator.cpp"
    ]
}
