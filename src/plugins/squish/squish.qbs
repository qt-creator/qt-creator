import qbs

QtcPlugin {
    name: "Squish"

    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }

    Depends { name: "Qt.widgets" }

    files: [
        "deletesymbolicnamedialog.cpp",
        "deletesymbolicnamedialog.h",
        "objectsmapdocument.cpp",
        "objectsmapdocument.h",
        "objectsmapeditor.cpp",
        "objectsmapeditor.h",
        "objectsmapeditorwidget.cpp",
        "objectsmapeditorwidget.h",
        "objectsmaptreeitem.cpp",
        "objectsmaptreeitem.h",
        "opensquishsuitesdialog.cpp",
        "opensquishsuitesdialog.h",
        "propertyitemdelegate.cpp",
        "propertyitemdelegate.h",
        "propertytreeitem.cpp",
        "propertytreeitem.h",
        "scripthelper.cpp",
        "scripthelper.h",
        "squishconstants.h",
        "squishfilehandler.cpp",
        "squishfilehandler.h",
        "squishmessages.cpp",
        "squishmessages.h",
        "squishnavigationwidget.cpp",
        "squishnavigationwidget.h",
        "squishoutputpane.cpp",
        "squishoutputpane.h",
        "squishperspective.cpp",
        "squishperspective.h",
        "squishplugin.cpp",
        "squishplugin_global.h",
        "squishprocessbase.cpp",
        "squishprocessbase.h",
        "squishresultmodel.cpp",
        "squishresultmodel.h",
        "squishrunnerprocess.cpp",
        "squishrunnerprocess.h",
        "squishserverprocess.cpp",
        "squishserverprocess.h",
        "squishsettings.cpp",
        "squishsettings.h",
        "squishtesttreemodel.cpp",
        "squishtesttreemodel.h",
        "squishtesttreeview.cpp",
        "squishtesttreeview.h",
        "squishtools.cpp",
        "squishtools.h",
        "squishtr.h",
        "squishwizardpages.cpp",
        "squishwizardpages.h",
        "squishxmloutputhandler.cpp",
        "squishxmloutputhandler.h",
        "suiteconf.cpp",
        "suiteconf.h",
        "symbolnameitemdelegate.cpp",
        "symbolnameitemdelegate.h",
        "testresult.cpp",
        "testresult.h",
    ]

    Group {
        name: "runtime resources"
        fileTags: "qt.core.resource_data"
        files: [
            "images/data.png",
            "images/data@2x.png",
            "images/jumpTo.png",
            "images/jumpTo@2x.png",
            "images/objectsmap.png",
            "images/objectsmap@2x.png",
            "images/picker.png",
            "images/picker@2x.png",
            "images/settingscategory_squish.png",
            "images/settingscategory_squish@2x.png",
            "wizard/suite/wizard.json",
        ]
    }
}
