QT += network
include(../../qtcreatorplugin.pri)

# GDB debug servers
include(debugservers/gdb/gdbservers.pri)
# UVSC debug servers
include(debugservers/uvsc/uvscservers.pri)

# BareMetal files

SOURCES += \
    baremetaldebugsupport.cpp \
    baremetaldevice.cpp \
    baremetaldeviceconfigurationwidget.cpp \
    baremetaldeviceconfigurationwizard.cpp \
    baremetaldeviceconfigurationwizardpages.cpp \
    baremetalplugin.cpp \
    baremetalrunconfiguration.cpp \
    debugserverproviderchooser.cpp \
    debugserverprovidermanager.cpp \
    debugserverproviderssettingspage.cpp \
    iarewparser.cpp \
    iarewtoolchain.cpp \
    idebugserverprovider.cpp \
    keilparser.cpp \
    keiltoolchain.cpp \
    sdccparser.cpp \
    sdcctoolchain.cpp

HEADERS += \
    baremetalconstants.h \
    baremetaldebugsupport.h \
    baremetaldevice.h \
    baremetaldeviceconfigurationwidget.h \
    baremetaldeviceconfigurationwizard.h \
    baremetaldeviceconfigurationwizardpages.h \
    baremetalplugin.h \
    baremetalrunconfiguration.h \
    debugserverproviderchooser.h \
    debugserverprovidermanager.h \
    debugserverproviderssettingspage.h \
    iarewparser.h \
    iarewtoolchain.h \
    idebugserverprovider.h \
    keilparser.h \
    keiltoolchain.h \
    sdccparser.h \
    sdcctoolchain.h

RESOURCES += \
    baremetal.qrc
