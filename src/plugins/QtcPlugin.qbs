import qbs.base 1.0
import qbs.fileinfo 1.0 as FileInfo

Product {
    type: ["dynamiclibrary", "pluginSpec"]
    destination: "lib/qtcreator/plugins/Nokia"

    Depends { name: "pluginspec" }
    Depends { name: 'cpp' }
    cpp.defines: [ name.toUpperCase() + "_LIBRARY" ]
    cpp.rpaths: ["$ORIGIN/../../.."]

    Group {
        files: [ name + ".pluginspec.in" ]
        fileTags: ["pluginSpecIn"]
    }
}

