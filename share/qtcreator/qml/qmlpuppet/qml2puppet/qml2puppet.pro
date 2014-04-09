TARGET = qml2puppet

TEMPLATE = app
CONFIG += console

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

include(qml2puppet.pri)
