include(../../qtcreatorplugin.pri)

QT += serialport

# SerialTerminal files

SOURCES += \
    consolelineedit.cpp \
    serialdevicemodel.cpp \
    serialoutputpane.cpp \
    serialterminalplugin.cpp \
    serialterminalsettings.cpp \
    serialcontrol.cpp

HEADERS += \
    consolelineedit.h \
    serialdevicemodel.h \
    serialoutputpane.h \
    serialterminalconstants.h \
    serialterminalplugin.h \
    serialterminalsettings.h \
    serialcontrol.h
