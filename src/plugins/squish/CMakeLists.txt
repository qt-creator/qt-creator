add_qtc_plugin(Squish
  PLUGIN_DEPENDS
    Core
  DEPENDS ExtensionSystem Utils
  SOURCES
    deletesymbolicnamedialog.cpp deletesymbolicnamedialog.h deletesymbolicnamedialog.ui
    objectsmapdocument.cpp objectsmapdocument.h
    objectsmapeditor.cpp objectsmapeditor.h
    objectsmapeditorwidget.cpp objectsmapeditorwidget.h
    objectsmaptreeitem.cpp objectsmaptreeitem.h
    opensquishsuitesdialog.cpp opensquishsuitesdialog.h opensquishsuitesdialog.ui
    propertyitemdelegate.cpp propertyitemdelegate.h
    propertytreeitem.cpp propertytreeitem.h
    squish.qrc
    squishfilehandler.cpp squishfilehandler.h
    squishnavigationwidget.cpp squishnavigationwidget.h
    squishoutputpane.cpp squishoutputpane.h
    squishplugin.cpp squishplugin.h
    squishresultmodel.cpp squishresultmodel.h
    squishsettings.cpp squishsettings.h
    squishtesttreemodel.cpp squishtesttreemodel.h
    squishtesttreeview.cpp squishtesttreeview.h
    squishtools.cpp squishtools.h
    squishutils.cpp squishutils.h
    squishxmloutputhandler.cpp squishxmloutputhandler.h
    symbolnameitemdelegate.cpp symbolnameitemdelegate.h
    testresult.cpp testresult.h
)
