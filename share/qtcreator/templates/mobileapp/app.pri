# This file should not be edited.
# Following versions of Qt Creator might offer new version.

INCLUDEPATH += $$PWD

defineTest(qtcAddDeployment) {
symbian {
    TARGET.EPOCHEAPSIZE = 0x20000 0x2000000
    contains(DEFINES, ORIENTATIONLOCK):LIBS += -lavkon -leikcore -leiksrv -lcone
    contains(DEFINES, NETWORKACCESS):TARGET.CAPABILITY += NetworkServices
} else:unix {
    maemo5 {
        installPrefix = /opt/usr
        desktopfile.path = /usr/share/applications/hildon
    } else {
        installPrefix = /usr/local
        desktopfile.path = /usr/share/applications
    }
    icon.files = $${TARGET}.png
    icon.path = /usr/share/icons/hicolor/64x64/apps
    desktopfile.files = $${TARGET}.desktop
    target.path = $${installPrefix}/bin
    export(icon.files)
    export(icon.path)
    export(desktopfile.files)
    export(desktopfile.path)
    export(target.path)
    INSTALLS += desktopfile icon target
}

export (INSTALLS)
export (TARGET.EPOCHEAPSIZE)
export (TARGET.CAPABILITY)
export (LIBS)

}
