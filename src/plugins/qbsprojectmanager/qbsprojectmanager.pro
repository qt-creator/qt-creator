include(../../qtcreatorplugin.pri)

# Look for qbs in the environment (again)
QBS_SOURCE_DIR = $$PWD/../../shared/qbs
QBS_BUILD_DIR = $$IDE_BUILD_TREE/src/shared/qbs

include($$QBS_SOURCE_DIR/src/lib/use.pri)
# Mac: fix rpath for qbscore soname
macx:QMAKE_LFLAGS += -Wl,-rpath,@loader_path/../

QBS_BUILD_DIR_FWD_SLASHES = $$replace(QBS_BUILD_DIR, \\\\, /)
DEFINES += QBS_BUILD_DIR=\\\"$$QBS_BUILD_DIR_FWD_SLASHES\\\"
DEFINES += \
    QBSPROJECTMANAGER_LIBRARY

HEADERS = \
    qbsbuildconfiguration.h \
    qbsbuildconfigurationwidget.h \
    qbsbuildstep.h \
    qbscleanstep.h \
    qbsdeployconfigurationfactory.h \
    qbsinstallstep.h \
    qbslogsink.h \
    qbsnodes.h \
    qbsparser.h \
    qbsproject.h \
    qbsprojectfile.h \
    qbsprojectmanager.h \
    qbsprojectmanager_global.h \
    qbsprojectmanagerconstants.h \
    qbsprojectmanagerplugin.h \
    qbspropertylineedit.h \
    qbsrunconfiguration.h \
    qbsstep.h

SOURCES = \
    qbsbuildconfiguration.cpp \
    qbsbuildconfigurationwidget.cpp \
    qbsbuildstep.cpp \
    qbscleanstep.cpp \
    qbsdeployconfigurationfactory.cpp \
    qbsinstallstep.cpp \
    qbslogsink.cpp \
    qbsnodes.cpp \
    qbsparser.cpp \
    qbsproject.cpp \
    qbsprojectfile.cpp \
    qbsprojectmanager.cpp \
    qbsprojectmanagerplugin.cpp \
    qbspropertylineedit.cpp \
    qbsrunconfiguration.cpp \
    qbsstep.cpp

FORMS = \
    qbsbuildstepconfigwidget.ui \
    qbscleanstepconfigwidget.ui \
    qbsinstallstepconfigwidget.ui
