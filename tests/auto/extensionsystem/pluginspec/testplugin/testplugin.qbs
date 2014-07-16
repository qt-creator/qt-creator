import qbs
import "../../plugin.qbs" as Plugin

Plugin {
    name: "pluginspec_test"
    files: [
        "testplugin.h", "testplugin.cpp",
        "testplugin_global.h"
    ]
    filesToCopy: "testplugin.xml"
    cpp.defines: base.concat(["MYPLUGIN_LIBRARY"])
}
