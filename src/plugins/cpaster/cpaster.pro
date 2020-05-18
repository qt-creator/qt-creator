QT += network
include(../../qtcreatorplugin.pri)
HEADERS += cpasterplugin.h \
    dpastedotcomprotocol.h \
    settingspage.h \
    protocol.h \
    pasteview.h \
    cpasterconstants.h \
    pastebindotcomprotocol.h \
    settings.h \
    pasteselectdialog.h \
    columnindicatortextedit.h \
    fileshareprotocol.h \
    fileshareprotocolsettingspage.h \
    stickynotespasteprotocol.h \
    urlopenprotocol.h \
    codepasterservice.h

SOURCES += cpasterplugin.cpp \
    dpastedotcomprotocol.cpp \
    settingspage.cpp \
    protocol.cpp \
    pasteview.cpp \
    pastebindotcomprotocol.cpp \
    settings.cpp \
    pasteselectdialog.cpp \
    columnindicatortextedit.cpp \
    fileshareprotocol.cpp \
    fileshareprotocolsettingspage.cpp \
    stickynotespasteprotocol.cpp \
    urlopenprotocol.cpp

FORMS += settingspage.ui \
    pasteselect.ui \
    pasteview.ui \
    pastebindotcomsettings.ui \
    fileshareprotocolsettingswidget.ui
include(../../shared/cpaster/cpaster.pri)

RESOURCES += \
    cpaster.qrc

DEFINES *= CPASTER_PLUGIN_GUI
