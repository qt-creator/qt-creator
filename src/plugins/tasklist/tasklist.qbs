import qbs 1.0

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
        "tasklistconstants.h",
        "tasklistplugin.cpp",
        "tasklistplugin.h",
    ]
}
