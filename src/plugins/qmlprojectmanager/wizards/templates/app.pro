QT += declarative

# MAINQML #
OTHER_FILES = qml/app.qml
SOURCES = cpp/main.cpp cpp/qmlapplicationview.cpp
HEADERS = cpp/qmlapplicationview.h
INCLUDEPATH += cpp

# DEPLOYMENTFOLDERS #
DEPLOYMENTFOLDERS = qml/app

# Avoid auto screen rotation
# ORIENTATIONLOCK #
DEFINES += ORIENTATIONLOCK

# Needs to be defined for Symbian
# NETWORKACCESS #
DEFINES += NETWORKACCESS

symbian {
    # TARGETUID3 #
    TARGET.UID3 = 0xE1111234
    ICON = cpp/symbianicon.svg
    for(deploymentfolder, DEPLOYMENTFOLDERS) {
        item = item$${deploymentfolder}
        itemsources = $${item}.sources
        $$itemsources = $${deploymentfolder}
        itempath = $${item}.path
        $$itempath = qml
        DEPLOYMENT += $$item
    }
    contains(DEFINES, ORIENTATIONLOCK):LIBS += -lavkon -leikcore -leiksrv -lcone
    contains(DEFINES, NETWORKACCESS):TARGET.CAPABILITY += NetworkServices
} else:win32 {
    # Ossi will want to kill me when he reads this
    # TODO: Let Ossi do some deployment-via-qmake magic
    !isEqual(PWD,$$OUT_PWD):!contains(CONFIG, build_pass) {
        copyCommand = @echo Copying Qml files...
        for(deploymentfolder, DEPLOYMENTFOLDERS) {
            pathSegments = $$split(deploymentfolder, /)
            sourceAndTarget = $$PWD/$$deploymentfolder $$OUT_PWD/qml/$$last(pathSegments)
            copyCommand += && $(COPY_DIR) $$replace(sourceAndTarget, /, \\)
        }
        copyqmlfiles.commands = $$copyCommand
        first.depends = $(first) copyqmlfiles
        QMAKE_EXTRA_TARGETS += first copyqmlfiles
    }
} else {
    # TODO: make this work
    for(deploymentfolder, DEPLOYMENTFOLDERS) {
        item = item$${deploymentfolder}
        itemfiles = $${item}.files
        $$itemfiles = $${deploymentfolder}
        itempath = $${item}.path
        $$itempath = qml
        INSTALLS += $$item
    }
}
