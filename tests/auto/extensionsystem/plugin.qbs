import qbs
import qbs.FileInfo
import QtcFunctions

DynamicLibrary {
    Depends { name: "Aggregation" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends { name: "copyable_resource" }
    targetName: QtcFunctions.qtLibraryName(qbs, name.split('_')[1])
    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)
    cpp.rpaths: [
        project.buildDirectory + "/" + project.libDirName + "/qtcreator",
        project.buildDirectory + "/" + project.libDirName + "/qtcreator/plugins"
    ].concat(additionalRPaths)
    cpp.cxxLanguageVersion: "c++11"
    property pathList additionalRPaths: []
}
