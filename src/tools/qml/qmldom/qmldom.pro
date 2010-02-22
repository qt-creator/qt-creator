#-------------------------------------------------
#
# Project created by QtCreator 2009-04-09T15:54:21
#
#-------------------------------------------------

!contains(QT_CONFIG, declarative) {
    error("Qt is not configured with the declarative model.");
}

QT       -= gui
QT += declarative

TARGET = qmldom
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp
