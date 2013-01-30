TEMPLATE = lib
TARGET = QbsProjectManager

include(../../qtcreatorplugin.pri)
include(../../libs/qmljs/qmljs.pri)
include(qbsprojectmanager_dependencies.pri)

# Look for qbs in the environment (again)
isEmpty(QBS_SOURCE_DIR): QBS_SOURCE_DIR = $$(QBS_SOURCE_DIR)
isEmpty(QBS_BUILD_DIR): QBS_BUILD_DIR = $$(QBS_BUILD_DIR)

QBSLIBDIR = $$QBS_BUILD_DIR/lib
include($$QBS_SOURCE_DIR/src/lib/use.pri)

DEFINES += QBS_SOURCE_DIR=\\\"$$QBS_SOURCE_DIR\\\"
DEFINES += QBS_BUILD_DIR=\\\"$$QBS_BUILD_DIR\\\"
DEFINES += \
    QT_CREATOR \
    QBSPROJECTMANAGER_LIBRARY \
    QT_NO_CAST_FROM_ASCII

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
    qbsprojectmanagerplugin.h

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
    qbsprojectmanagerplugin.cpp

FORMS = \
    qbsbuildstepconfigwidget.ui \
    qbscleanstepconfigwidget.ui

