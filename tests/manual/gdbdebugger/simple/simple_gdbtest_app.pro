TEMPLATE = app
TARGET = simple_gdbtest_app
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = .

# Input
SOURCES += \
    simple_gdbtest_app.cpp

# SOURCES += ../../../share/qtcreator/gdbmacros/gdbmacros.cpp
QT += network
message("this says <foo & bar>")
