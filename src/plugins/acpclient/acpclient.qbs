import qbs

QtcPlugin {
    name: "AcpClient"

    Depends { name: "Qt.core" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.network" }
    Depends { name: "Utils" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Acp" }

    files: [
        "acpchatwidget.cpp",
        "acpchatwidget.h",
        "acpclientconstants.h",
        "acpclientobject.cpp",
        "acpclientobject.h",
        "acpclientplugin.cpp",
        "acpclienttr.h",
        "acpconnectionwidget.cpp",
        "acpconnectionwidget.h",
        "acpfilesystemhandler.cpp",
        "acpfilesystemhandler.h",
        "acpmessageview.cpp",
        "acpmessageview.h",
        "acppermissionhandler.cpp",
        "acppermissionhandler.h",
        "acpstdiotransport.cpp",
        "acpstdiotransport.h",
        "acptcptransport.cpp",
        "acptcptransport.h",
        "acpterminalhandler.cpp",
        "acpterminalhandler.h",
        "acptransport.cpp",
        "acptransport.h",
    ]
}
