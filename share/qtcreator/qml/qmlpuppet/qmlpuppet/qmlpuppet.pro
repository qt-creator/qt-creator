TARGET = qmlpuppet

TEMPLATE = app

DESTDIR = $$[QT_INSTALL_BINS]

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

include(../../../../../share/qtcreator/qml/qmlpuppet/qmlpuppet/qmlpuppet.pri)