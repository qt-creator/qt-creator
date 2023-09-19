import qbs.FileInfo

QtcManualTest {
    name: "Manual test plugin2"
    targetName: "plugin2"
    type: [ "dynamiclibrary" ]
    destinationDirectory: FileInfo.cleanPath(FileInfo.joinPaths(base , ".."))

    Depends { name: "ExtensionSystem" }

    files: [
        "plugin2.cpp",
        "plugin2.h"
    ]
}
