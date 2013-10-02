import qbs.base 1.0

import QtcPlugin

QtcPlugin {
    name: "TaskList"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Find" }
    Depends { name: "Locator" }
    Depends { name: "TextEditor" }

    files: [
        "stopmonitoringhandler.cpp",
        "stopmonitoringhandler.h",
        "taskfile.cpp",
        "taskfile.h",
        "taskfilefactory.cpp",
        "taskfilefactory.h",
        "tasklist.qrc",
        "tasklistconstants.h",
        "tasklistplugin.cpp",
        "tasklistplugin.h",
    ]
}
