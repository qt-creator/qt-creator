QT_BUILD_TREE=$$(QT_BUILD_TREE)
isEmpty(QT_BUILD_TREE):QT_BUILD_TREE=$$(QTDIR)
QT_QRC_BUILD_TREE = $$fromfile($$QT_BUILD_TREE/.qmake.cache,QT_SOURCE_TREE)

INCLUDEPATH *= $$QT_QRC_BUILD_TREE/tools/designer/src/lib/shared
INCLUDEPATH *= $$PWD $$PWD/..

QT *= xml

DEFINES *= QT_NO_SHARED_EXPORT

# Input
SOURCES += \
    $$PWD/resourcefile.cpp \
    $$PWD/resourceview.cpp \
    $$PWD/qrceditor.cpp \
    $$PWD/undocommands.cpp \

HEADERS += \
    $$PWD/resourcefile_p.h \
    $$PWD/resourceview.h \
    $$PWD/qrceditor.h \
    $$PWD/undocommands_p.h \
    \
    $$PWD/../namespace_global.h \

FORMS += $$PWD/qrceditor.ui

