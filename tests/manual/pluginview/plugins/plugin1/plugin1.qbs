import qbs.FileInfo

QtcManualTest {
    name: "Manual test plugin1"
    targetName: "plugin1"
    type: [ "dynamiclibrary" ]
    destinationDirectory: FileInfo.cleanPath(FileInfo.joinPaths(base , ".."))

    Depends { name: "ExtensionSystem" }
    Depends { name: "Manual test plugin2"}
    Depends { name: "Manual test plugin3"}

    files: [
        "plugin1.cpp",
        "plugin1.h"
    ]
}
