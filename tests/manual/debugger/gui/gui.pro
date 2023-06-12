TARGET = gui
CONFIG+=console
QT *= widgets

TEMPLATE = app
SOURCES += \
    mainwindow.cpp \
    tst_gui.cpp

HEADERS += mainwindow.h
FORMS += mainwindow.ui

macos: QMAKE_POST_LINK = codesign -f -s - --entitlements "$${PWD}/entitlements.plist" "$${OUT_PWD}/gui.app"
