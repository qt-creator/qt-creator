import qbs

QtcPlugin {
    name: "Squish"

    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }

    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.core5compat" }

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
        "squish.qrc",
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
}
