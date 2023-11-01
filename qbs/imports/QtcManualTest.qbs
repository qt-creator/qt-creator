import qbs.FileInfo

QtcProduct {
    condition: qtc.withAutotests
    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)
    targetName: "tst_" + name.split(' ').join("")
    type: "application"

    install: false

    Depends { name: "Qt.testlib" }
    Depends { name: "copyable_resource" }

    cpp.defines: {
        var defines = base.filter(function(d) { return d !== "QT_RESTRICTED_CAST_FROM_ASCII"; });
        return defines;
    }
    cpp.rpaths: [
        project.buildDirectory + '/' + qtc.ide_library_path,
        project.buildDirectory + '/' + qtc.ide_plugin_path
    ]
}
