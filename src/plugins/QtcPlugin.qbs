import qbs.base 1.0
import qbs.fileinfo 1.0 as FileInfo

Product {
    type: ["dynamiclibrary", "pluginSpec"]
    destination: "lib/qtcreator/plugins/Nokia"
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
    Depends { name: 'cpp' }
    cpp.defines: [name.toUpperCase() + "_LIBRARY"]
    cpp.rpaths: ["$ORIGIN/../../.."]

    Group {
        files: [ name + ".pluginspec.in" ]
        fileTags: ["pluginSpecIn"]
    }
}

