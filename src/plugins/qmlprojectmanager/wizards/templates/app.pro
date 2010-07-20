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
        eval(item$${deploymentfolder}.sources = $${deploymentfolder})
        eval(item$${deploymentfolder}.path = qml)
        eval(DEPLOYMENT += item$${deploymentfolder})
    }
    contains(DEFINES, ORIENTATIONLOCK):LIBS += -lavkon -leikcore -leiksrv -lcone
    contains(DEFINES, NETWORKACCESS):TARGET.CAPABILITY += NetworkServices
} else:win32 {
    # Ossi will want to kill me when he reads this
    # TODO: let Ossi create a (post link step) deployment for windows
    !contains(CONFIG, build_pass):for(deploymentfolder, DEPLOYMENTFOLDERS) {
        pathSegments = $$split(deploymentfolder, /)
        sourceAndTarget = $$deploymentfolder $$OUT_PWD/qml/$$last(pathSegments)
        system($$QMAKE_COPY_DIR $$replace(sourceAndTarget, /, \\))
    }
} else {
    # TODO: make this work
    for(deploymentfolder, DEPLOYMENTFOLDERS) {
        eval(item$${deploymentfolder}.files = $${deploymentfolder})
        eval(item$${deploymentfolder}.path = qml)
        eval(INSTALLS += item$${deploymentfolder})
    }
}
