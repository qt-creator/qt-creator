import qbs.FileInfo
import qbs.Utilities
import QtcFunctions

DynamicLibrary {
    Depends { name: "Aggregation" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends { name: "copyable_resource" }
    Depends { name: "qtc" }
    targetName: QtcFunctions.qtLibraryName(qbs, name.split('_')[1])
    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)
    cpp.cxxFlags: {
        var flags = [];
        if (qbs.toolchain.contains("clang")
                && !qbs.hostOS.contains("darwin")
                && Utilities.versionCompare(cpp.compilerVersion, "10") >= 0) {
             // Triggers a lot in Qt.
            flags.push("-Wno-deprecated-copy", "-Wno-constant-logical-operand");
        }
        return flags;
    }
    cpp.rpaths: [
        project.buildDirectory + "/" + qtc.libDirName + "/qtcreator",
        project.buildDirectory + "/" + qtc.libDirName + "/qtcreator/plugins"
    ].concat(additionalRPaths)
    cpp.cxxLanguageVersion: "c++11"
    property pathList additionalRPaths: []
}
