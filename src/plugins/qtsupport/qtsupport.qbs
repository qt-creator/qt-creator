import qbs 1.0

Project {
    name: "QtSupport"

    QtcDevHeaders { }

    QtcPlugin {
        Depends { name: "Qt"; submodules: ["quick", "widgets", "xml"]; }
        Depends { name: "QmlJS" }
        Depends { name: "Utils" }

        Depends { name: "Core" }
        Depends { name: "ProParser" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "CppTools" }

        files: [
            "baseqtversion.cpp",
            "baseqtversion.h",
            "codegenerator.cpp",
            "codegenerator.h",
            "codegensettings.cpp",
            "codegensettings.h",
            "codegensettingspage.cpp",
            "codegensettingspage.h",
            "codegensettingspagewidget.ui",
            "qtconfigwidget.cpp",
            "qtconfigwidget.h",
            "qtsupport.qrc",
            "exampleslistmodel.cpp",
            "exampleslistmodel.h",
            "profilereader.cpp",
            "profilereader.h",
            "qmldumptool.cpp",
            "qmldumptool.h",
            "qscxmlcgenerator.cpp",
            "qscxmlcgenerator.h",
            "qtkitconfigwidget.cpp",
            "qtkitconfigwidget.h",
            "qtkitinformation.cpp",
            "qtkitinformation.h",
            "qtoptionspage.cpp",
            "qtoptionspage.h",
            "qtoutputformatter.cpp",
            "qtoutputformatter.h",
            "qtparser.cpp",
            "qtparser.h",
            "qtsupport_global.h",
            "qtsupportconstants.h",
            "qtsupportplugin.cpp",
            "qtsupportplugin.h",
            "qtversionfactory.cpp",
            "qtversionfactory.h",
            "qtversioninfo.ui",
            "qtversionmanager.cpp",
            "qtversionmanager.h",
            "qtversionmanager.ui",
            "screenshotcropper.cpp",
            "screenshotcropper.h",
            "showbuildlog.ui",
            "uicgenerator.cpp",
            "uicgenerator.h",
        ]

        Group {
            name: "QtVersion"
            files: [
                "desktopqtversion.cpp", "desktopqtversion.h",
                "desktopqtversionfactory.cpp", "desktopqtversionfactory.h",
                "winceqtversion.cpp", "winceqtversion.h",
                "winceqtversionfactory.cpp", "winceqtversionfactory.h",
            ]
        }

        Group {
            name: "Getting Started Welcome Page"
            files: [
                "gettingstartedwelcomepage.cpp",
                "gettingstartedwelcomepage.h"
            ]
        }
    }
}
