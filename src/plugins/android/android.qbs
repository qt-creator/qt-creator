import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Android"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Qt4ProjectManager" }
    Depends { name: "Debugger" }
    Depends { name: "QtSupport" }
    Depends { name: "Qt"; submodules: ["widgets", "xml", "network"] }

    property bool enable: false
    property var pluginspecreplacements: ({"ANDROID_EXPERIMENTAL_STR": (enable ? "false": "true")})

    cpp.includePaths: [
        "..",
        buildDirectory,
        "../../libs",
        "../../shared"
    ]

    files: [
        "addnewavddialog.ui",
        "androidconfigurations.cpp",
        "androidconfigurations.h",
        "androidconstants.h",
        "androidcreatekeystorecertificate.cpp",
        "androidcreatekeystorecertificate.h",
        "androidcreatekeystorecertificate.ui",
        "androiddebugsupport.cpp",
        "androiddebugsupport.h",
        "androiddeployconfiguration.cpp",
        "androiddeployconfiguration.h",
        "androiddeploystep.cpp",
        "androiddeploystepfactory.cpp",
        "androiddeploystepfactory.h",
        "androiddeploystep.h",
        "androiddeploystepwidget.cpp",
        "androiddeploystepwidget.h",
        "androiddeploystepwidget.ui",
        "androidglobal.h",
        "androidmanager.h",
        "androidmanager.cpp",
        "androidpackagecreationfactory.cpp",
        "androidpackagecreationfactory.h",
        "androidpackagecreationstep.cpp",
        "androidpackagecreationstep.h",
        "androidpackagecreationwidget.cpp",
        "androidpackagecreationwidget.h",
        "androidpackagecreationwidget.ui",
        "androidpackageinstallationfactory.cpp",
        "androidpackageinstallationfactory.h",
        "androidpackageinstallationstep.cpp",
        "androidpackageinstallationstep.h",
        "androidplugin.cpp",
        "androidplugin.h",
        "android.qrc",
        "androidqtversion.cpp",
        "androidqtversionfactory.cpp",
        "androidqtversionfactory.h",
        "androidqtversion.h",
        "androidrunconfiguration.cpp",
        "androidrunconfiguration.h",
        "androidruncontrol.cpp",
        "androidruncontrol.h",
        "androidrunfactories.cpp",
        "androidrunfactories.h",
        "androidrunner.cpp",
        "androidrunner.h",
        "androidsettingspage.cpp",
        "androidsettingspage.h",
        "androidsettingswidget.cpp",
        "androidsettingswidget.h",
        "androidsettingswidget.ui",
        "androidtoolchain.cpp",
        "androidtoolchain.h",
        "javaparser.cpp",
        "javaparser.h"
    ]
}
