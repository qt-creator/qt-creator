include(../../qtcreatorplugin.pri)

# Look for qbs in the environment
isEmpty(QBS_INSTALL_DIR): QBS_INSTALL_DIR = $$(QBS_INSTALL_DIR)
isEmpty(QBS_INSTALL_DIR) {
    QBS_SOURCE_DIR = $$PWD/../../shared/qbs
    include($$QBS_SOURCE_DIR/src/lib/corelib/use_corelib.pri)
    include($$QBS_SOURCE_DIR/src/lib/qtprofilesetup/use_qtprofilesetup.pri)
    osx:QMAKE_LFLAGS += -Wl,-rpath,@loader_path/../Frameworks # OS X: fix rpath for qbscore soname
} else {
    include($${QBS_INSTALL_DIR}/include/qbs/use_installed_corelib.pri)
    include($${QBS_INSTALL_DIR}/include/qbs/use_installed_qtprofilesetup.pri)
}
QBS_INSTALL_DIR_FWD_SLASHES = $$replace(QBS_INSTALL_DIR, \\\\, /)
DEFINES += QBS_INSTALL_DIR=\\\"$$QBS_INSTALL_DIR_FWD_SLASHES\\\"

DEFINES += \
    QBSPROJECTMANAGER_LIBRARY

HEADERS = \
    customqbspropertiesdialog.h \
    defaultpropertyprovider.h \
    propertyprovider.h \
    qbsbuildconfiguration.h \
    qbsbuildconfigurationwidget.h \
    qbsbuildinfo.h \
    qbsbuildstep.h \
    qbscleanstep.h \
    qbsdeployconfigurationfactory.h \
    qbskitinformation.h \
    qbsinstallstep.h \
    qbslogsink.h \
    qbsnodes.h \
    qbsnodetreebuilder.h \
    qbsparser.h \
    qbspmlogging.h \
    qbsprofilessettingspage.h \
    qbsproject.h \
    qbsprojectimporter.h \
    qbsprojectmanager.h \
    qbsprojectmanager_global.h \
    qbsprojectmanagerconstants.h \
    qbsprojectmanagerplugin.h \
    qbsprojectmanagersettings.h \
    qbsprojectparser.h \
    qbsrunconfiguration.h

SOURCES = \
    customqbspropertiesdialog.cpp \
    defaultpropertyprovider.cpp \
    qbsbuildconfiguration.cpp \
    qbsbuildconfigurationwidget.cpp \
    qbsbuildinfo.cpp \
    qbsbuildstep.cpp \
    qbscleanstep.cpp \
    qbsdeployconfigurationfactory.cpp \
    qbsinstallstep.cpp \
    qbskitinformation.cpp \
    qbslogsink.cpp \
    qbsnodes.cpp \
    qbsnodetreebuilder.cpp \
    qbsparser.cpp \
    qbspmlogging.cpp \
    qbsprofilessettingspage.cpp \
    qbsproject.cpp \
    qbsprojectimporter.cpp \
    qbsprojectmanager.cpp \
    qbsprojectmanagerplugin.cpp \
    qbsprojectmanagersettings.cpp \
    qbsprojectparser.cpp \
    qbsrunconfiguration.cpp

FORMS = \
    customqbspropertiesdialog.ui \
    qbsbuildstepconfigwidget.ui \
    qbscleanstepconfigwidget.ui \
    qbsinstallstepconfigwidget.ui \
    qbsprofilessettingswidget.ui

RESOURCES += \
   qbsprojectmanager.qrc
