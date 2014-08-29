TARGET = gui
CONFIG+=console
QT *= widgets

TEMPLATE = app
SOURCES += \
    mainwindow.cpp \
    tst_gui.cpp

HEADERS += mainwindow.h
FORMS += mainwindow.ui
