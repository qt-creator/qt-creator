TARGET = qml2puppet

TEMPLATE = app

DESTDIR = $$[QT_INSTALL_BINS]

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

include(../../../../../share/qtcreator/qml/qmlpuppet/qml2puppet/qml2puppet.pri)
