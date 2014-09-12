TEMPLATE = lib
TARGET = myplugin
QT += qml
CONFIG += qt plugin

TARGET = $$qtLibraryTarget($$TARGET)
uri = qquick1

# Input
SOURCES += \
    myplugin.cpp \
    mytype.cpp

HEADERS += \
    myplugin.h \
    mytype.h

DISTFILES = qmldir

!equals(_PRO_FILE_PWD_, $$OUT_PWD) {
    copy_qmldir.target = $$OUT_PWD/qmldir
    copy_qmldir.depends = $$_PRO_FILE_PWD_/qmldir
    copy_qmldir.commands = $(COPY_FILE) \"$$replace(copy_qmldir.depends, /, $$QMAKE_DIR_SEP)\" \"$$replace(copy_qmldir.target, /, $$QMAKE_DIR_SEP)\"
    QMAKE_EXTRA_TARGETS += copy_qmldir
    PRE_TARGETDEPS += $$copy_qmldir.target
}
