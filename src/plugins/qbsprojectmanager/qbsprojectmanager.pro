include(../../qtcreatorplugin.pri)

# Look for qbs in the environment
isEmpty(QBS_INSTALL_DIR): QBS_INSTALL_DIR = $$(QBS_INSTALL_DIR)
isEmpty(QBS_INSTALL_DIR) {
    QBS_SOURCE_DIR = $$PWD/../../shared/qbs
    include($$QBS_SOURCE_DIR/src/lib/use.pri)
    macx:QMAKE_LFLAGS += -Wl,-rpath,@loader_path/../ # Mac: fix rpath for qbscore soname
} else {
    include($${QBS_INSTALL_DIR}/include/qbs/use_installed.pri)
}
QBS_INSTALL_DIR_FWD_SLASHES = $$replace(QBS_INSTALL_DIR, \\\\, /)
DEFINES += QBS_INSTALL_DIR=\\\"$$QBS_INSTALL_DIR_FWD_SLASHES\\\"

DEFINES += \
    QBSPROJECTMANAGER_LIBRARY

HEADERS = \
    defaultpropertyprovider.h \
    propertyprovider.h \
    qbsbuildconfiguration.h \
    qbsbuildconfigurationwidget.h \
    qbsbuildinfo.h \
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
    qbsstep.h \
    qbsconstants.h

SOURCES = \
    defaultpropertyprovider.cpp \
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

RESOURCES += \
   qbsprojectmanager.qrc
