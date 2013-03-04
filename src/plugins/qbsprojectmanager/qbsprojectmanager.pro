TEMPLATE = lib
TARGET = QbsProjectManager

include(../../qtcreatorplugin.pri)
include(../../libs/qmljs/qmljs.pri)
include(qbsprojectmanager_dependencies.pri)

# Look for qbs in the environment (again)
QBS_SOURCE_DIR = $$PWD/../../shared/qbs
QBS_BUILD_DIR = $$IDE_BUILD_TREE/src/shared/qbs

QBSLIBDIR = $$QBS_BUILD_DIR/lib
include($$QBS_SOURCE_DIR/src/lib/use.pri)
linux-*:QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$$QBSLIBDIR\'
macx:QMAKE_LFLAGS += -Wl,-rpath,$$QBSLIBDIR

QBS_BUILD_DIR_FWD_SLASHES = $$replace(QBS_BUILD_DIR, \\\\, /)
DEFINES += QBS_BUILD_DIR=\\\"$$QBS_BUILD_DIR_FWD_SLASHES\\\"
DEFINES += \
    QT_CREATOR \
    QBSPROJECTMANAGER_LIBRARY

HEADERS = \
    qbsbuildconfiguration.h \
    qbsbuildconfigurationwidget.h \
    qbsbuildstep.h \
    qbscleanstep.h \
    qbslogsink.h \
    qbsnodes.h \
    qbsparser.h \
    qbsproject.h \
    qbsprojectfile.h \
    qbsprojectmanager.h \
    qbsprojectmanager_global.h \
    qbsprojectmanagerconstants.h \
    qbsprojectmanagerplugin.h \
    qbsstep.h

SOURCES = \
    qbsbuildconfiguration.cpp \
    qbsbuildconfigurationwidget.cpp \
    qbsbuildstep.cpp \
    qbscleanstep.cpp \
    qbslogsink.cpp \
    qbsnodes.cpp \
    qbsparser.cpp \
    qbsproject.cpp \
    qbsprojectfile.cpp \
    qbsprojectmanager.cpp \
    qbsprojectmanagerplugin.cpp \
    qbsstep.cpp

FORMS = \
    qbsbuildstepconfigwidget.ui \
    qbscleanstepconfigwidget.ui \
    qbsstepconfigwidget.ui
