import qbs
import qbs.FileInfo
import "./copytransformer.qbs" as CopyTransformer
import QtcFunctions

DynamicLibrary {
    Depends { name: "Aggregation" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    targetName: QtcFunctions.qtLibraryName(qbs, name.split('_')[1])
    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)
    cpp.rpaths: [
        project.buildDirectory + "/" + project.libDirName + "/qtcreator",
        project.buildDirectory + "/" + project.libDirName + "/qtcreator/plugins"
    ].concat(additionalRPaths)
    property pathList filesToCopy
    property pathList additionalRPaths: []
    CopyTransformer {
        sourceFiles: product.filesToCopy
        targetDirectory: product.destinationDirectory
    }
}
