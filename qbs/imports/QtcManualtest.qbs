import qbs
import qbs.FileInfo

QtcProduct {
    type: ["application"]

    Depends { name: "Qt.testlib" }
    Depends { name: "copyable_resource" }
    targetName: "tst_" + name.split(' ').join("")

    cpp.rpaths: [
        project.buildDirectory + '/' + qtc.ide_library_path,
        project.buildDirectory + '/' + qtc.ide_plugin_path
    ]
    cpp.defines: {
        var defines = base.filter(function(d) { return d !== "QT_RESTRICTED_CAST_FROM_ASCII"; });
        return defines;
    }

    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)
    install: false
}
