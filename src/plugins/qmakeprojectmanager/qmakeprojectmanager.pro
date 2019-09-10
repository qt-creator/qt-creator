QT += network
include(../../qtcreatorplugin.pri)

DEFINES += \
    QMAKEPROJECTMANAGER_LIBRARY

HEADERS += \
    qmakebuildinfo.h \
    qmakekitinformation.h \
    qmakeparsernodes.h \
    qmakeprojectimporter.h \
    qmakeprojectmanagerplugin.h \
    qmakeprojectmanager.h \
    qmakeproject.h \
    qmakesettings.h \
    qmakenodes.h \
    qmakenodetreebuilder.h \
    profileeditor.h \
    profilehighlighter.h \
    profilehoverhandler.h \
    wizards/qtprojectparameters.h \
    wizards/filespage.h \
    wizards/qtwizard.h \
    wizards/subdirsprojectwizard.h \
    wizards/subdirsprojectwizarddialog.h \
    wizards/simpleprojectwizard.h \
    qmakeprojectmanagerconstants.h \
    qmakestep.h \
    qmakeprojectconfigwidget.h \
    externaleditors.h \
    qmakebuildconfiguration.h \
    qmakeparser.h \
    addlibrarywizard.h \
    librarydetailscontroller.h \
    qmakeprojectmanager_global.h \
    profilecompletionassist.h \
    makefileparse.h \
    qmakemakestep.h

SOURCES += \
    qmakekitinformation.cpp \
    qmakeparsernodes.cpp \
    qmakeprojectimporter.cpp \
    qmakeprojectmanagerplugin.cpp \
    qmakeprojectmanager.cpp \
    qmakeproject.cpp \
    qmakenodes.cpp \
    qmakesettings.cpp \
    qmakenodetreebuilder.cpp \
    profileeditor.cpp \
    profilehighlighter.cpp \
    profilehoverhandler.cpp \
    wizards/qtprojectparameters.cpp \
    wizards/filespage.cpp \
    wizards/qtwizard.cpp \
    wizards/subdirsprojectwizard.cpp \
    wizards/subdirsprojectwizarddialog.cpp \
    wizards/simpleprojectwizard.cpp \
    qmakestep.cpp \
    qmakeprojectconfigwidget.cpp \
    externaleditors.cpp \
    qmakebuildconfiguration.cpp \
    qmakeparser.cpp \
    addlibrarywizard.cpp \
    librarydetailscontroller.cpp \
    profilecompletionassist.cpp \
    makefileparse.cpp \
    qmakemakestep.cpp

FORMS += \
    qmakestep.ui \
    librarydetailswidget.ui

RESOURCES += qmakeprojectmanager.qrc \
    wizards/wizards.qrc

include(customwidgetwizard/customwidgetwizard.pri)
