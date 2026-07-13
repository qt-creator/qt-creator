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
  LONG_DESCRIPTION_MD AcpClientDescription.md
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
    acpterminalhandler.cpp acpterminalhandler.h
    acptransport.cpp acptransport.h
    chatinputcompletion.cpp chatinputcompletion.h
    chatinputedit.cpp chatinputedit.h
    chatpanel.cpp chatpanel.h
    sessionpickerwidget.cpp sessionpickerwidget.h
    collapsibleframe.cpp collapsibleframe.h
    toolcalldetailwidget.cpp toolcalldetailwidget.h
)

qtc_add_resources(AcpClient "images"
  PREFIX "/acpclient"
  BASE "."
  FILES
    images/sparklebubble.png
    images/sparklebubble@2x.png
)

extend_qtc_plugin(AcpClient
  CONDITION WITH_TESTS
  DEFINES "ACP_TESTSERVER_DIR=\"${CMAKE_CURRENT_BINARY_DIR}/testserver\""
  SOURCES
    acpclienttest.cpp acpclienttest.h
)

if(WITH_TESTS)
  add_subdirectory(testserver)
endif()
