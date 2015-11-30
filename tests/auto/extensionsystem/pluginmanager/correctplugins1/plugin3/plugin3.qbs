import qbs
import "../../../plugin.qbs" as Plugin

Plugin {
    name: "correct_plugin3"
    Depends { name: "correct_plugin2" }
    additionalRPaths: [destinationDirectory + "/../plugin2"]
    files: ["plugin3.h", "plugin3.cpp"]
    cpp.defines: base.concat(["PLUGIN3_LIBRARY"])
}
