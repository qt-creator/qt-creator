import qbs 1.0

QtcPlugin {
    name: "Ios"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QmakeProjectManager" }
    Depends { name: "Debugger" }
    Depends { name: "QtSupport" }
    Depends { name: "QmlDebug" }
    Depends { name: "Qt"; submodules: ["widgets", "xml", "network"] }

    cpp.frameworks: base.concat(qbs.targetOS.contains("macos") ? ["CoreFoundation", "IOKit"] : [])

    files: [
        "createsimulatordialog.cpp",
        "createsimulatordialog.h",
        "createsimulatordialog.ui",
        "ios.qrc",
        "iosbuildconfiguration.cpp",
        "iosbuildconfiguration.h",
        "iosbuildstep.cpp",
        "iosbuildstep.h",
        "iosconfigurations.cpp",
        "iosconfigurations.h",
        "iosconstants.h",
        "iosdeploystep.cpp",
        "iosdeploystep.h",
        "iosdevice.cpp",
        "iosdevice.h",
        "iosdsymbuildstep.cpp",
        "iosdsymbuildstep.h",
        "iosplugin.cpp",
        "iosplugin.h",
        "iospresetbuildstep.ui",
        "iosprobe.cpp",
        "iosprobe.h",
        "iosqtversion.cpp",
        "iosqtversion.h",
        "iosrunconfiguration.cpp",
        "iosrunconfiguration.h",
        "iosrunner.cpp",
        "iosrunner.h",
        "iossettingspage.cpp",
        "iossettingspage.h",
        "iossettingswidget.cpp",
        "iossettingswidget.h",
        "iossettingswidget.ui",
        "iossimulator.cpp",
        "iossimulator.h",
        "iostoolhandler.cpp",
        "iostoolhandler.h",
        "simulatorcontrol.cpp",
        "simulatorcontrol.h",
        "simulatorinfomodel.cpp",
        "simulatorinfomodel.h",
        "simulatoroperationdialog.cpp",
        "simulatoroperationdialog.h",
        "simulatoroperationdialog.ui"
    ]
}
