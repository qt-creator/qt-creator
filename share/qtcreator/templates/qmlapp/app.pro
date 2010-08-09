# Add more folders to ship with the application, here
# DEPLOYMENTFOLDERS #
folder_01.source = qml/app
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01
# DEPLOYMENTFOLDERS_END #

# Avoid auto screen rotation
# ORIENTATIONLOCK #
DEFINES += ORIENTATIONLOCK

# Needs to be defined for Symbian
# NETWORKACCESS #
DEFINES += NETWORKACCESS

# MAINQML #
OTHER_FILES = qml/app/app.qml

# TARGETUID3 #
symbian:TARGET.UID3 = 0xE1111234

# Define to enable the Qml Inspector in debug mode
# QMLINSPECTOR #
#DEFINES += QMLINSPECTOR

# Path to the Qml Inspector sources, which are shipped with Qt Creator
# QMLINSPECTOR_PATH #
QMLINSPECTOR_PATH = $$PWD/../../qmljsdebugger

# Edit the code below on your own risk.

QT += declarative

SOURCES += cpp/main.cpp cpp/qmlapplicationview.cpp
HEADERS += cpp/qmlapplicationview.h
INCLUDEPATH += cpp

contains(DEFINES, QMLINSPECTOR):CONFIG(debug, debug|release):include($$QMLINSPECTOR_PATH/qmljsdebugger-lib.pri)

symbian {
    ICON = cpp/symbianicon.svg
    contains(DEFINES, ORIENTATIONLOCK):LIBS += -lavkon -leikcore -leiksrv -lcone
    contains(DEFINES, NETWORKACCESS):TARGET.CAPABILITY += NetworkServices
    for(deploymentfolder, DEPLOYMENTFOLDERS) {
        item = item$${deploymentfolder}
        itemsources = $${item}.sources
        $$itemsources = $$eval($${deploymentfolder}.source)
        itempath = $${item}.path
        $$itempath= $$eval($${deploymentfolder}.target)
        DEPLOYMENT += $$item
    }
} else:win32 {
    !isEqual(PWD,$$OUT_PWD) {
        copyCommand = @echo Copying application data...
        for(deploymentfolder, DEPLOYMENTFOLDERS) {
            source = $$eval($${deploymentfolder}.source)
            pathSegments = $$split(source, /)
            sourceAndTarget = $$PWD/$$source $$OUT_PWD/$$eval($${deploymentfolder}.target)/$$last(pathSegments)
            copyCommand += && $(COPY_DIR) $$replace(sourceAndTarget, /, \\)
        }
        copydeploymentfolders.commands = $$copyCommand
        first.depends = $(first) copydeploymentfolders
        QMAKE_EXTRA_TARGETS += first copydeploymentfolders
    }
} else:if(maemo5|maemo6) {
    for(deploymentfolder, DEPLOYMENTFOLDERS) {
        item = item$${deploymentfolder}
        itemfiles = $${item}.files
        $$itemfiles = $$eval($${deploymentfolder}.source)
        itempath = $${item}.path
        $$itempath = /opt/share/$$eval($${deploymentfolder}.target)
        INSTALLS += $$item
    }
    desktopfile.files = $${TARGET}.desktop
    desktopfile.path = /usr/share/applications/hildon
    target.path = /opt/bin
    INSTALLS += desktopfile target
} else:macx {
    !isEqual(PWD,$$OUT_PWD) {
        copyCommand = @echo Copying application data...
        for(deploymentfolder, DEPLOYMENTFOLDERS) {
            target = $$OUT_PWD/$${TARGET}.app/Contents/Resources/$$eval($${deploymentfolder}.target)
            copyCommand += && $(MKDIR) $$target
            copyCommand += && $(COPY_DIR) $$PWD/$$eval($${deploymentfolder}.source) $$target
        }
        copydeploymentfolders.commands = $$copyCommand
        first.depends = $(first) copydeploymentfolders
        QMAKE_EXTRA_TARGETS += first copydeploymentfolders
    }
} else { #linux
    !isEqual(PWD,$$OUT_PWD) {
        copyCommand = @echo Copying application data...
        for(deploymentfolder, DEPLOYMENTFOLDERS) {
            target = $$OUT_PWD/$$eval($${deploymentfolder}.target)
            copyCommand += && $(MKDIR) $$target
            copyCommand += && $(COPY_DIR) $$PWD/$$eval($${deploymentfolder}.source) $$target
        }
        copydeploymentfolders.commands = $$copyCommand
        first.depends = $(first) copydeploymentfolders
        QMAKE_EXTRA_TARGETS += first copydeploymentfolders
    }
}
