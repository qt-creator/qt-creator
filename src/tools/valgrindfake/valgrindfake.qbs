import qbs
import qbs.Utilities

CppApplication {
    name: "valgrind-fake"
    consoleApplication: true
    destinationDirectory: qtc.ide_bin_path
    Depends { name: "Qt"; submodules: ["network", "xml"]; }
    Depends { name: "qtc" }
    cpp.cxxLanguageVersion: "c++11"

    Properties {
        condition: Utilities.versionCompare(Qt.core.version, "5.7") < 0
        cpp.minimumMacosVersion: project.minimumMacosVersion
    }

    files: [
        "main.cpp",
        "outputgenerator.h", "outputgenerator.cpp"
    ]
}
