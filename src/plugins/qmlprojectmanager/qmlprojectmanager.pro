TEMPLATE = lib
TARGET = QmlProjectManager

QT += declarative

include(../../qtcreatorplugin.pri)
include(qmlprojectmanager_dependencies.pri)
include(fileformat/fileformat.pri)

DEFINES += QMLPROJECTMANAGER_LIBRARY
HEADERS += qmlproject.h \
    qmlprojectplugin.h \
    qmlprojectmanager.h \
    qmlprojectconstants.h \
    qmlprojectnodes.h \
    qmlprojectimportwizard.h \
    qmlprojectfile.h \
    qmlprojectruncontrol.h \
    qmlprojectrunconfiguration.h \
    qmlprojectrunconfigurationfactory.h \
    qmlprojectapplicationwizard.h \
    qmlprojectmanager_global.h \
    qmlprojectmanagerconstants.h \
    qmlprojecttarget.h \
    wizards/qmlstandaloneappwizard.h \
    wizards/qmlstandaloneappwizardpages.h \
    wizards/qmlstandaloneapp.h

SOURCES += qmlproject.cpp \
    qmlprojectplugin.cpp \
    qmlprojectmanager.cpp \
    qmlprojectnodes.cpp \
    qmlprojectimportwizard.cpp \
    qmlprojectfile.cpp \
    qmlprojectruncontrol.cpp \
    qmlprojectrunconfiguration.cpp \
    qmlprojectrunconfigurationfactory.cpp \
    qmlprojectapplicationwizard.cpp \
    qmlprojecttarget.cpp \
    wizards/qmlstandaloneappwizard.cpp \
    wizards/qmlstandaloneappwizardpages.cpp \
    wizards/qmlstandaloneapp.cpp

RESOURCES += qmlproject.qrc

INCLUDEPATH += \
    . \
    wizards

FORMS += \
    wizards/qmlstandaloneappwizardoptionspage.ui \
    wizards/qmlstandaloneappwizardsourcespage.ui

OTHER_FILES += QmlProjectManager.pluginspec \
               QmlProject.mimetypes.xml
