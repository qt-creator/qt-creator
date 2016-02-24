import qbs

QtcAutotest {
    Depends { name: "Debugger" }
    Depends { name: "Core" }
    Depends { name: "QtcSsh" }
    Depends { name: "Utils" }
    Depends { name: "ProjectExplorer" }
    property path pluginDir: project.ide_source_tree + "/src/plugins/valgrind"

    Group {
        name: "XML protocol files from plugin"
        prefix: product.pluginDir + "/xmlprotocol/"
        files: ["*.h", "*.cpp"]
    }
    Group {
        name: "Callgrind files from plugin"
        prefix: product.pluginDir + "/callgrind/"
        files: ["*.h", "*.cpp"]
    }
    Group {
        name: "Memcheck runner files from plugin"
        prefix: product.pluginDir + "/memcheck/"
        files: ["*.h", "*.cpp"]
    }
    Group {
        name: "Other files from plugin"
        prefix: product.pluginDir + "/"
        files: [
            "valgrindprocess.h", "valgrindprocess.cpp",
            "valgrindrunner.h", "valgrindrunner.cpp",
        ]
    }
    cpp.includePaths: base.concat([project.ide_source_tree + "/src/plugins"])
}
