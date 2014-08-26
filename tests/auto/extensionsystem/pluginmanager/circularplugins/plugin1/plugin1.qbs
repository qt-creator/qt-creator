import qbs
import "../../../plugin.qbs" as Plugin

Plugin {
    name: "circular_plugin1"
    files: ["plugin1.h", "plugin1.cpp"]
    cpp.defines: base.concat(["PLUGIN1_LIBRARY"])
}
