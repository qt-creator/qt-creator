add_subdirectory(frontend)

add_qtc_plugin(CodePaster
  PLUGIN_DEPENDS Core TextEditor
  DEFINES CPASTER_PLUGIN_GUI
  DEPENDS Qt5::Network
  INCLUDES ../../shared/cpaster
  SOURCES
    codepasterservice.h
    columnindicatortextedit.cpp columnindicatortextedit.h
    cpasterconstants.h
    cpaster.qrc
    cpasterplugin.cpp cpasterplugin.h
    dpastedotcomprotocol.cpp dpastedotcomprotocol.h
    fileshareprotocol.cpp fileshareprotocol.h
    fileshareprotocolsettingspage.cpp fileshareprotocolsettingspage.h
    fileshareprotocolsettingswidget.ui
    frontend/argumentscollector.cpp frontend/argumentscollector.h
    frontend/main.cpp
    pastebindotcomprotocol.cpp pastebindotcomprotocol.h
    pastebindotcomsettings.ui
    pasteselect.ui
    pasteselectdialog.cpp pasteselectdialog.h
    pasteview.cpp pasteview.h pasteview.ui
    protocol.cpp protocol.h
    settings.cpp settings.h
    settingspage.cpp settingspage.h settingspage.ui
    stickynotespasteprotocol.cpp stickynotespasteprotocol.h
    urlopenprotocol.cpp urlopenprotocol.h

    ../../shared/cpaster/cgi.cpp ../../shared/cpaster/cgi.h
    ../../shared/cpaster/splitter.cpp ../../shared/cpaster/splitter.h
)
