import qbs
import "../../../plugin.qbs" as Plugin

Plugin {
    name: "correct_plugin2"
    filesToCopy: "plugin.spec"
    files: ["plugin2.h", "plugin2.cpp"].concat(filesToCopy)
    cpp.defines: base.concat(["PLUGIN2_LIBRARY"])
}
