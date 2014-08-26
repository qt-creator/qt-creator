import qbs
import "../../../plugin.qbs" as Plugin

Plugin {
    name: "circular_plugin2"
    files: ["plugin2.h", "plugin2.cpp"]
    cpp.defines: base.concat(["PLUGIN2_LIBRARY"])
}
