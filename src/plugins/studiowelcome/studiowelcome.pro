DEFINES += STUDIOWELCOME_LIBRARY

QT += quick quickwidgets

include(../../qtcreatorplugin.pri)

## magic fix for unix builds
RCC_DIR = .

DEFINES += STUDIO_QML_PATH=\\\"$$PWD/qml/\\\"

HEADERS += \
    studiowelcome_global.h \
    studiowelcomeplugin.h \
    newprojectdialogimageprovider.h \
    qdsnewdialog.h \
    wizardfactories.h \
    createproject.h \
    newprojectmodel.h \
    examplecheckout.h

SOURCES += \
    studiowelcomeplugin.cpp \
    qdsnewdialog.cpp \
    wizardfactories.cpp \
    createproject.cpp \
    newprojectdialogimageprovider.cpp \
    newprojectmodel.cpp \
    examplecheckout.cpp

OTHER_FILES += \
    StudioWelcome.json.in

RESOURCES += \
    ../../share/3rdparty/studiofonts/studiofonts.qrc \
    studiowelcome.qrc

RESOURCES += \
    $$files(qml/*)
