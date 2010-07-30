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




# Edit the code below on your own risk.

QT += declarative

SOURCES = cpp/main.cpp cpp/qmlapplicationview.cpp
HEADERS = cpp/qmlapplicationview.h
INCLUDEPATH += cpp

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
            target = $$eval($${deploymentfolder}.target)
            pathSegments = $$split(source, /)
            sourceAndTarget = $$PWD/$$source $$OUT_PWD/$$target/$$last(pathSegments)
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
    target.path = /opt/bin
    INSTALLS += target
} else:macx {
    for(deploymentfolder, DEPLOYMENTFOLDERS) {
        item = item$${deploymentfolder}
        itemsources = $${item}.files
        $$itemsources = $$eval($${deploymentfolder}.source)
        itempath = $${item}.path
        $$itempath = Resources/$$eval($${deploymentfolder}.target)
        QMAKE_BUNDLE_DATA += $$item
    }
} else { #linux
    !isEqual(PWD,$$OUT_PWD) {
        copyCommand = @echo Copying application data...
        for(deploymentfolder, DEPLOYMENTFOLDERS) {
            source = $$eval($${deploymentfolder}.source)
            target = $$OUT_PWD/$$eval($${deploymentfolder}.target)/$$last(pathSegments)
            copyCommand += && $(MKDIR) $$target
            copyCommand += && $(COPY_DIR) $$PWD/$$source $$target
        }
        copydeploymentfolders.commands = $$copyCommand
        first.depends = $(first) copydeploymentfolders
        QMAKE_EXTRA_TARGETS += first copydeploymentfolders
    }
}
