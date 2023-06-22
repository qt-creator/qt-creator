import qbs 1.0

Product {
    condition: qbs.targetOS.contains("linux")
    name: "LogoImages"

    Group {
        name: "16x16"
        qbs.install: true
        qbs.installDir: "share/icons/hicolor/16x16/apps"
        files: ["16/QtProject-qtcreator.png"]
    }

    Group {
        name: "24x24"
        qbs.install: true
        qbs.installDir: "share/icons/hicolor/24x24/apps"
        files: ["24/QtProject-qtcreator.png"]
    }

    Group {
        name: "32x32"
        qbs.install: true
        qbs.installDir: "share/icons/hicolor/32x32/apps"
        files: ["32/QtProject-qtcreator.png"]
    }

    Group {
        name: "48x48"
        qbs.install: true
        qbs.installDir: "share/icons/hicolor/48x48/apps"
        files: ["48/QtProject-qtcreator.png"]
    }

    Group {
        name: "64x64"
        qbs.install: true
        qbs.installDir: "share/icons/hicolor/64x64/apps"
        files: ["64/QtProject-qtcreator.png"]
    }

    Group {
        name: "128x128"
        qbs.install: true
        qbs.installDir: "share/icons/hicolor/128x128/apps"
        files: ["128/QtProject-qtcreator.png"]
    }

    Group {
        name: "256x256"
        qbs.install: true
        qbs.installDir: "share/icons/hicolor/256x256/apps"
        files: ["256/QtProject-qtcreator.png"]
    }

    Group {
        name: "512x512"
        qbs.install: true
        qbs.installDir: "share/icons/hicolor/512x512/apps"
        files: ["512/QtProject-qtcreator.png"]
    }
}
