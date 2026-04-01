import qbs

QtcPlugin {
    name: "AcpClient"

    Depends { name: "Qt"; submodules: ["core", "widgets", "network"] }
    Depends { name: "AcpLib" }
    Depends { name: "Utils" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }

    files: [
        "acpchatcontroller.cpp",
        "acpchatcontroller.h",
        "acpchattab.cpp",
        "acpchattab.h",
        "acpchatwidget.cpp",
        "acpchatwidget.h",
        "acpclientconstants.h",
        "acpclientobject.cpp",
        "acpclientobject.h",
        "acpclientplugin.cpp",
        "acpclienttr.h",
        "acpfilesystemhandler.cpp",
        "acpfilesystemhandler.h",
        "acpinspector.cpp",
        "acpinspector.h",
        "acpmessageview.cpp",
        "acpmessageview.h",
        "acppermissionhandler.cpp",
        "acppermissionhandler.h",
        "acpsettings.cpp",
        "acpsettings.h",
        "acpstdiotransport.cpp",
        "acpstdiotransport.h",
        "acptcptransport.cpp",
        "acptcptransport.h",
        "acpterminalhandler.cpp",
        "acpterminalhandler.h",
        "acptransport.cpp",
        "acptransport.h",
        "chatinputedit.cpp",
        "chatinputedit.h",
        "chatpanel.cpp",
        "chatpanel.h",
        "collapsibleframe.h",
        "toolcalldetailwidget.cpp",
        "toolcalldetailwidget.h",
    ]
}
