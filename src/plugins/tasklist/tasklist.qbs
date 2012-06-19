import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "TaskList"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Find" }
    Depends { name: "Locator" }
    Depends { name: "TextEditor" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "tasklistplugin.h",
        "tasklist_export.h",
        "tasklistconstants.h",
        "stopmonitoringhandler.h",
        "taskfile.h",
        "taskfilefactory.h",
        "tasklistplugin.cpp",
        "stopmonitoringhandler.cpp",
        "taskfile.cpp",
        "taskfilefactory.cpp",
        "tasklist.qrc",
        "TaskList.mimetypes.xml"
    ]
}

