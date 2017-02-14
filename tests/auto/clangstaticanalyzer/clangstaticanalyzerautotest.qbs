import qbs

QtcAutotest {
    Depends { name: "Qt.widgets" }
    Depends { name: "Debugger" }
    Depends { name: "Utils" }

    property path pluginDir: project.ide_source_tree + "/src/plugins/clangstaticanalyzer"
    cpp.includePaths: base.concat(pluginDir + "/..")
}
