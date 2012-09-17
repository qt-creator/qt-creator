import qbs.base 1.0
import qbs.fileinfo 1.0 as FileInfo

Product {
    type: ["dynamiclibrary", "pluginSpec"]
    property string provider: 'Nokia'
    property var pluginspecreplacements
    destination: "lib/qtcreator/plugins/" + provider
    targetName: {
        // see PluginSpecPrivate::loadLibrary()
        if (qbs.debugInformation) {
            switch (qbs.targetOS) {
            case "windows":
                return name + "d";
            case "mac":
                return name + "_debug";
            }
        }
        return name;
    }

    Depends { name: "pluginspec" }
    Depends { name: "cpp" }
    cpp.defines: project.additionalCppDefines.concat([name.toUpperCase() + "_LIBRARY"])
    cpp.rpaths: ["$ORIGIN/../../.."]
    cpp.linkerFlags: {
        if (qbs.buildVariant == "release" && (qbs.toolchain == "gcc" || qbs.toolchain == "mingw"))
            return ["-Wl,-s"]
    }

    Group {
        files: [ name + ".pluginspec.in" ]
        fileTags: ["pluginSpecIn"]
    }
}

