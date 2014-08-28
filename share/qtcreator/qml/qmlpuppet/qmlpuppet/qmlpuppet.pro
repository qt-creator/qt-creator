TARGET = qmlpuppet

TEMPLATE = app

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

include(qmlpuppet.pri)
