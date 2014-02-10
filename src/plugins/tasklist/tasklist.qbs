import qbs.base 1.0

import QtcPlugin

QtcPlugin {
    name: "TaskList"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

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
