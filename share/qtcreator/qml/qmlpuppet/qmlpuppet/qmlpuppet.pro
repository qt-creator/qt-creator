TARGET = qmlpuppet

TEMPLATE = app

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

greaterThan(QT_MAJOR_VERSION, 4) {
    DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x040900
}

include(qmlpuppet.pri)
