QT += network
include(../../qtcreatorplugin.pri)
HEADERS += cpasterplugin.h \
    settingspage.h \
    protocol.h \
    pasteview.h \
    cpasterconstants.h \
    pastebindotcomprotocol.h \
    pastebindotcaprotocol.h \
    settings.h \
    pasteselectdialog.h \
    columnindicatortextedit.h \
    fileshareprotocol.h \
    fileshareprotocolsettingspage.h \
    kdepasteprotocol.h \
    urlopenprotocol.h \
    codepasterservice.h \
    authenticationdialog.h

SOURCES += cpasterplugin.cpp \
    settingspage.cpp \
    protocol.cpp \
    pasteview.cpp \
    pastebindotcomprotocol.cpp \
    pastebindotcaprotocol.cpp \
    settings.cpp \
    pasteselectdialog.cpp \
    columnindicatortextedit.cpp \
    fileshareprotocol.cpp \
    fileshareprotocolsettingspage.cpp \
    kdepasteprotocol.cpp \
    urlopenprotocol.cpp \
    authenticationdialog.cpp

FORMS += settingspage.ui \
    pasteselect.ui \
    pasteview.ui \
    pastebindotcomsettings.ui \
    fileshareprotocolsettingswidget.ui
include(../../shared/cpaster/cpaster.pri)

RESOURCES += \
    cpaster.qrc

DEFINES *= CPASTER_PLUGIN_GUI
