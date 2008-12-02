TEMPLATE = app
TARGET = installercreator
DEPENDPATH += .
INCLUDEPATH += .

CONFIG -= debug
CONFIG += release

HEADERS += \
    qinstaller.h \
    qinstallergui.h \

SOURCES += \
    qinstaller.cpp \
    qinstallergui.cpp \
    installer.cpp \

RESOURCES += \
    installer.qrc \

true {
    OBJECTS_DIR = .tmp/
    MOC_DIR = .tmp/
    RCC_DIR = .tmp/
    UI_DIR = .tmp/
}

win32:LIBS += ole32.lib

