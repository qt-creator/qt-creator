DEFINES += STUDIOWELCOME_LIBRARY

QT += quick quickwidgets

include(../../qtcreatorplugin.pri)

## magic fix for unix builds
RCC_DIR = .

DEFINES += STUDIO_QML_PATH=\\\"$$PWD/qml/\\\"

HEADERS += \
    studiowelcome_global.h \
    studiowelcomeplugin.h \
    examplecheckout.h

SOURCES += \
    studiowelcomeplugin.cpp \
    examplecheckout.cpp

OTHER_FILES += \
    StudioWelcome.json.in

RESOURCES += \
    ../../share/3rdparty/studiofonts/studiofonts.qrc \
    studiowelcome.qrc

RESOURCES += \
    $$files(qml/*)
