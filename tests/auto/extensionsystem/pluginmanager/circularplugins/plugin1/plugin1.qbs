import qbs
import "../../../plugin.qbs" as Plugin

Plugin {
    name: "circular_plugin1"
    filesToCopy: "plugin.xml"
    files: ["plugin1.h", "plugin1.cpp"].concat(filesToCopy)
    cpp.defines: base.concat(["PLUGIN1_LIBRARY"])
}
