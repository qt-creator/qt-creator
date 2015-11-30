import qbs
import "../../../plugin.qbs" as Plugin

Plugin {
    name: "circular_plugin3"
    files: ["plugin3.h", "plugin3.cpp"]
    cpp.defines: base.concat(["PLUGIN3_LIBRARY"])
}
