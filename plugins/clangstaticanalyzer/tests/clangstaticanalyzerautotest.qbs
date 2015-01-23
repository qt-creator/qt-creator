import qbs

QtcAutotest {
    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    property path pluginDir: "../../"
    cpp.defines: base.concat('SRCDIR="' + sourceDirectory + '"')
    cpp.includePaths: base.concat(pluginDir + "/..")
}
