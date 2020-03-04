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
    qmakeproject.h \
    qmakesettings.h \
    qmakenodes.h \
    qmakenodetreebuilder.h \
    profileeditor.h \
    profilehighlighter.h \
    profilehoverhandler.h \
    wizards/qtprojectparameters.h \
    wizards/qtwizard.h \
    wizards/subdirsprojectwizard.h \
    wizards/subdirsprojectwizarddialog.h \
    qmakeprojectmanagerconstants.h \
    qmakestep.h \
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
    qmakeproject.cpp \
    qmakenodes.cpp \
    qmakesettings.cpp \
    qmakenodetreebuilder.cpp \
    profileeditor.cpp \
    profilehighlighter.cpp \
    profilehoverhandler.cpp \
    wizards/qtprojectparameters.cpp \
    wizards/qtwizard.cpp \
    wizards/subdirsprojectwizard.cpp \
    wizards/subdirsprojectwizarddialog.cpp \
    qmakestep.cpp \
    externaleditors.cpp \
    qmakebuildconfiguration.cpp \
    qmakeparser.cpp \
    addlibrarywizard.cpp \
    librarydetailscontroller.cpp \
    profilecompletionassist.cpp \
    makefileparse.cpp \
    qmakemakestep.cpp

FORMS += \
    librarydetailswidget.ui

RESOURCES += qmakeprojectmanager.qrc \
    wizards/wizards.qrc

include(customwidgetwizard/customwidgetwizard.pri)
