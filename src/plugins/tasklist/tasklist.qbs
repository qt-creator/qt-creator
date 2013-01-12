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

    files: [
        "TaskList.mimetypes.xml",
        "stopmonitoringhandler.cpp",
        "stopmonitoringhandler.h",
        "taskfile.cpp",
        "taskfile.h",
        "taskfilefactory.cpp",
        "taskfilefactory.h",
        "tasklist.qrc",
        "tasklist_export.h",
        "tasklistconstants.h",
        "tasklistplugin.cpp",
        "tasklistplugin.h",
    ]
}
