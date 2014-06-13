# for testing with terminal
QT       += core
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle

# for testing without terminal
#QT       += core gui

TARGET = cli-io
TEMPLATE = app
SOURCES += main.cpp
