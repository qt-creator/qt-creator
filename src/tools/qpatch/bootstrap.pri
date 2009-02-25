CONFIG 	       += console qtinc
CONFIG         -= qt
build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}
CONFIG     -= app_bundle

DEFINES	       += \
        QT_BOOTSTRAPPED \
        QT_LITE_UNICODE \
        QT_TEXTCODEC \
        QT_NO_CAST_FROM_ASCII \
        QT_NO_CAST_TO_ASCII \
        QT_NO_CODECS \
        QT_NO_DATASTREAM \
        QT_NO_GEOM_VARIANT \
        QT_NO_LIBRARY \
        QT_NO_QOBJECT \
        QT_NO_STL \
        QT_NO_SYSTEMLOCALE \
        QT_NO_TEXTSTREAM \
        QT_NO_THREAD \
        QT_NO_UNICODETABLES \
        QT_NO_USING_NAMESPACE
win32:DEFINES += QT_NODLL

INCLUDEPATH	+= $$QT_BUILD_TREE/include \
                   $$QT_BUILD_TREE/include/QtCore \
                   $$QT_BUILD_TREE/include/QtXml \
                   $$QT_SOURCE_TREE/src/xml
DEPENDPATH	+= $$INCLUDEPATH \
                   $$QT_SOURCE_TREE/src/corelib/global \
                   $$QT_SOURCE_TREE/src/corelib/kernel \
                   $$QT_SOURCE_TREE/src/corelib/tools \
                   $$QT_SOURCE_TREE/src/corelib/io \
                   $$QT_SOURCE_TREE/src/corelib/codecs \
                   $$QT_SOURCE_TREE/src/xml

hpux-acc*|hpuxi-acc* {
    LIBS += $$QT_BUILD_TREE/src/tools/bootstrap/libbootstrap.a
} else {
    contains(CONFIG, debug_and_release_target) {
        CONFIG(debug, debug|release) {
            LIBS+=-L$$QT_BUILD_TREE/src/tools/bootstrap/debug
        } else {
            LIBS+=-L$$QT_BUILD_TREE/src/tools/bootstrap/release
        }
    } else {
        LIBS += -L$$QT_BUILD_TREE/src/tools/bootstrap
    }
    LIBS += -lbootstrap
}
!contains(QT_CONFIG, zlib):!contains(QT_CONFIG, no-zlib) {
   unix:LIBS += -lz
#  win32:LIBS += libz.lib
}
win32:LIBS += -luser32

mac {
    CONFIG -= incremental
    LIBS += -framework CoreServices
}

