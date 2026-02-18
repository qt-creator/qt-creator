add_qtc_plugin(AcpClient
  PLUGIN_DEPENDS
    QtCreator::Core
    QtCreator::ProjectExplorer
    QtCreator::TextEditor
  DEPENDS
    Qt::Widgets
    Qt::Network
    QtCreator::ExtensionSystem
    QtCreator::Utils
  PUBLIC_DEPENDS
    AcpLib
  SOURCES
    acpchatcontroller.cpp acpchatcontroller.h
    acpchattab.cpp acpchattab.h
    acpchatwidget.cpp acpchatwidget.h
    acpclientconstants.h
    acpinspector.cpp acpinspector.h
    acpclientobject.cpp acpclientobject.h
    acpclientplugin.cpp
    acpclienttr.h
    acpsettings.cpp acpsettings.h
    acpfilesystemhandler.cpp acpfilesystemhandler.h
    acpmessageview.cpp acpmessageview.h
    acppermissionhandler.cpp acppermissionhandler.h
    acpstdiotransport.cpp acpstdiotransport.h
    acptcptransport.cpp acptcptransport.h
    acpterminalhandler.cpp acpterminalhandler.h
    acptransport.cpp acptransport.h
    chatinputedit.cpp chatinputedit.h
    chatpanel.cpp chatpanel.h
    collapsibleframe.h
    toolcalldetailwidget.cpp toolcalldetailwidget.h
)
