import qbs
import "../../../plugin.qbs" as Plugin

Plugin {
    name: "correct_plugin1"
    Depends { name: "correct_plugin2" }
    Depends { name: "correct_plugin3" }
    additionalRPaths: [
        destinationDirectory + "/../plugin2",
        destinationDirectory + "/../plugin3"
    ]
    files: ["plugin1.h", "plugin1.cpp"]
    cpp.defines: base.concat(["PLUGIN1_LIBRARY"])
}
