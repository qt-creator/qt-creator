add_qtc_plugin(Squish
  PLUGIN_DEPENDS
    Core Debugger ProjectExplorer TextEditor
  DEPENDS ExtensionSystem Utils
  SOURCES
    deletesymbolicnamedialog.cpp deletesymbolicnamedialog.h
    objectsmapdocument.cpp objectsmapdocument.h
    objectsmapeditor.cpp objectsmapeditor.h
    objectsmapeditorwidget.cpp objectsmapeditorwidget.h
    objectsmaptreeitem.cpp objectsmaptreeitem.h
    opensquishsuitesdialog.cpp opensquishsuitesdialog.h
    propertyitemdelegate.cpp propertyitemdelegate.h
    propertytreeitem.cpp propertytreeitem.h
    scripthelper.cpp scripthelper.h
    squish.qrc
    squishfilehandler.cpp squishfilehandler.h
    squishmessages.cpp squishmessages.h
    squishnavigationwidget.cpp squishnavigationwidget.h
    squishoutputpane.cpp squishoutputpane.h
    squishperspective.cpp squishperspective.h
    squishplugin.cpp
    squishprocessbase.cpp squishprocessbase.h
    squishresultmodel.cpp squishresultmodel.h
    squishrunnerprocess.cpp squishrunnerprocess.h
    squishsettings.cpp squishsettings.h
    squishserverprocess.cpp squishserverprocess.h
    squishtesttreemodel.cpp squishtesttreemodel.h
    squishtesttreeview.cpp squishtesttreeview.h
    squishtools.cpp squishtools.h
    squishtr.h
    squishwizardpages.cpp squishwizardpages.h
    squishxmloutputhandler.cpp squishxmloutputhandler.h
    suiteconf.cpp suiteconf.h
    symbolnameitemdelegate.cpp symbolnameitemdelegate.h
    testresult.cpp testresult.h
)
