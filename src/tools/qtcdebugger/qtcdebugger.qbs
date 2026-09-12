import qbs 1.0

QtcTool {
    name: "qtcdebugger"
    windowsFileDescription: qtc.ide_display_name + " Debug Dispatcher"
    condition: qbs.targetOS.contains("windows")

    property string registryAccessDir: project.sharedSourcesDir + "/registryaccess"

    cpp.includePaths: base.concat([registryAccessDir])
    cpp.dynamicLibraries: [
        "psapi",
        "advapi32"
    ]

    Depends { name: "Qt.widgets" }
    Depends { name: "app_version_header" }

    files: [
        "main.cpp",
        registryAccessDir + "/registryaccess.cpp",
        registryAccessDir + "/registryaccess.h",
    ]
}
