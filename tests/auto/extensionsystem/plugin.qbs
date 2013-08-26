import qbs
import qbs.File
import qbs.FileInfo
import "./copytransformer.qbs" as CopyTransformer
import "../../../qbs/defaults.js" as Defaults

DynamicLibrary {
    Depends { name: "Aggregation" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    targetName: Defaults.qtLibraryName(qbs, name.split('_')[1])
    destinationDirectory: FileInfo.relativePath(project.ide_source_tree, sourceDirectory)
    cpp.rpaths: [
        buildDirectory + "/lib/qtcreator",
        buildDirectory + "/lib/qtcreator/plugins/QtProject"
    ].concat(additionalRPaths)
    property pathList filesToCopy
    property pathList additionalRPaths: []
    CopyTransformer {
        sourceFiles: product.filesToCopy
        targetDirectory: product.destinationDirectory
    }
}
