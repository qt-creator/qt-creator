import qbs
import qbs.FileInfo

QtcProduct {
    condition: qtc.withAutotests

    // This needs to be absolute, because it is passed to one of the source files.
    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)

    targetName: "tst_" + name.split(' ').join("")
    type: ["application", "autotest"]

    installTags: []

    Depends { name: "autotest" }
    Depends { name: "Qt.testlib" }
    Depends { name: "copyable_resource" }

    cpp.defines: {
        var defines = base.filter(function(d) { return d !== "QT_RESTRICTED_CAST_FROM_ASCII"; });
        var absLibExecPath = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix,
                                                qtc.ide_libexec_path);
        var relLibExecPath = FileInfo.relativePath(destinationDirectory, absLibExecPath);
        defines.push('TEST_RELATIVE_LIBEXEC_PATH="' + relLibExecPath + '"');
        return defines;
    }
    cpp.rpaths: [
        project.buildDirectory + '/' + qtc.ide_library_path,
        project.buildDirectory + '/' + qtc.ide_plugin_path
    ]

    // Qt Creator's generated headers and unity-style translation units routinely
    // hold more sections than the object format takes by default, which MSVC and
    // MinGW's assembler both reject with a fatal error.
    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.cxxFlags: "/bigobj"
    }
    Properties {
        condition: qbs.toolchain.contains("mingw")
        cpp.cxxFlags: "-Wa,-mbig-obj"
    }

    // The following would be conceptually right, but does not work currently as some autotests
    // (e.g. extensionsystem) do not work when installed, because they want hardcoded
    // absolute paths to resources in the build directory.
    //    cpp.rpaths: qbs.targetOS.contains("macos")
    //            ? ["@loader_path/../Frameworks", "@loader_path/../PlugIns"]
    //            : ["$ORIGIN/../" + qtc.libDirName + "/qtcreator",
    //               "$ORIGIN/../" qtc.libDirName + "/qtcreator/plugins"]
}
