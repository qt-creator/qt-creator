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
        "iosbuildsettingswidget.cpp",
        "iosbuildsettingswidget.h",
        "iosbuildsettingswidget.ui",
        "iosbuildstep.cpp",
        "iosbuildstep.h",
        "iosbuildstep.ui",
        "iosconfigurations.cpp",
        "iosconfigurations.h",
        "iosconstants.h",
        "iosdeployconfiguration.cpp",
        "iosdeployconfiguration.h",
        "iosdeploystep.cpp",
        "iosdeploystep.h",
        "iosdeploystepfactory.cpp",
        "iosdeploystepfactory.h",
        "iosdeploystepwidget.cpp",
        "iosdeploystepwidget.h",
        "iosdeploystepwidget.ui",
        "iosdevice.cpp",
        "iosdevice.h",
        "iosdevicefactory.cpp",
        "iosdevicefactory.h",
        "iosdsymbuildstep.cpp",
        "iosdsymbuildstep.h",
        "iosplugin.cpp",
        "iosplugin.h",
        "iospresetbuildstep.ui",
        "iosprobe.cpp",
        "iosprobe.h",
        "iosqtversion.cpp",
        "iosqtversion.h",
        "iosqtversionfactory.cpp",
        "iosqtversionfactory.h",
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
        "iossimulatorfactory.cpp",
        "iossimulatorfactory.h",
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
