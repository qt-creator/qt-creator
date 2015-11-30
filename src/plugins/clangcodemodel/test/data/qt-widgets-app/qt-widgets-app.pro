# "Qt Widgets Application" from Qt Creator's template

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qt-widgets-app
TEMPLATE = app

SOURCES += main.cpp mainwindow.cpp
HEADERS += mainwindow.h
FORMS += mainwindow.ui
