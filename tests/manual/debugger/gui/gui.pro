TARGET = gui
CONFIG+=console
greaterThan(QT_MAJOR_VERSION, 4):QT *= widgets

TEMPLATE = app
SOURCES += \
    mainwindow.cpp \
    tst_gui.cpp

HEADERS += mainwindow.h
FORMS += mainwindow.ui
