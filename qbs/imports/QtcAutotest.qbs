import qbs
import qbs.FileInfo

QtcProduct {
    type: ["application", "autotest"]
    Depends { name: "Qt.test" }
    Depends { name: "copyable_resource" }
    targetName: "tst_" + name.split(' ').join("")

    // This needs to be absolute, because it is passed to one of the source files.
    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)

    cpp.rpaths: [
        project.buildDirectory + '/' + project.ide_library_path,
        project.buildDirectory + '/' + project.ide_plugin_path
    ]
    cpp.minimumOsxVersion: "10.7"
    cpp.defines: base.filter(function(d) { return d != "QT_NO_CAST_FROM_ASCII"; })

    Group {
        fileTagsFilter: product.type
        qbs.install: false
    }

    // The following would be conceptually right, but does not work currently as some autotests
    // (e.g. extensionsystem) do not work when installed, because they want hardcoded
    // absolute paths to resources in the build directory.
    //    cpp.rpaths: qbs.targetOS.contains("osx")
    //            ? ["@loader_path/../Frameworks", "@loader_path/../PlugIns"]
    //            : ["$ORIGIN/../" + project.libDirName + "/qtcreator",
    //               "$ORIGIN/../" project.libDirName + "/qtcreator/plugins"]
}
