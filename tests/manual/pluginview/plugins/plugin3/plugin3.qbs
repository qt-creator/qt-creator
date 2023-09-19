import qbs.FileInfo

QtcManualTest {
    name: "Manual test plugin3"
    targetName: "plugin3"
    type: [ "dynamiclibrary" ]
    destinationDirectory: FileInfo.cleanPath(FileInfo.joinPaths(base , ".."))

    Depends { name: "ExtensionSystem" }
    Depends { name: "Manual test plugin2" }

    files: [
        "plugin3.cpp",
        "plugin3.h"
    ]
}
