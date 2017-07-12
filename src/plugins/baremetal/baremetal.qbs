import qbs

QtcPlugin {
    name: "BareMetal"

    Depends { name: "Qt"; submodules: ["network", "widgets"]; }
    Depends { name: "QtcSsh" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }

    files: [
        "baremetal.qrc",
        "baremetalconstants.h",
        "baremetalcustomrunconfiguration.cpp", "baremetalcustomrunconfiguration.h",
        "baremetaldevice.cpp", "baremetaldevice.h",
        "baremetaldeviceconfigurationfactory.cpp", "baremetaldeviceconfigurationfactory.h",
        "baremetaldeviceconfigurationwidget.cpp", "baremetaldeviceconfigurationwidget.h",
        "baremetaldeviceconfigurationwizard.cpp", "baremetaldeviceconfigurationwizard.h",
        "baremetaldeviceconfigurationwizardpages.cpp", "baremetaldeviceconfigurationwizardpages.h",
        "baremetalgdbcommandsdeploystep.cpp", "baremetalgdbcommandsdeploystep.h",
        "baremetalplugin.cpp", "baremetalplugin.h",
        "baremetalrunconfiguration.cpp", "baremetalrunconfiguration.h",
        "baremetalrunconfigurationfactory.cpp", "baremetalrunconfigurationfactory.h",
        "baremetalrunconfigurationwidget.cpp", "baremetalrunconfigurationwidget.h",
        "baremetaldebugsupport.cpp", "baremetaldebugsupport.h",
        "gdbserverproviderprocess.cpp", "gdbserverproviderprocess.h",
        "gdbserverproviderssettingspage.cpp", "gdbserverproviderssettingspage.h",
        "gdbserverprovider.cpp", "gdbserverprovider.h",
        "gdbserverproviderchooser.cpp", "gdbserverproviderchooser.h",
        "gdbserverprovidermanager.cpp", "gdbserverprovidermanager.h",
        "openocdgdbserverprovider.cpp", "openocdgdbserverprovider.h",
        "defaultgdbserverprovider.cpp", "defaultgdbserverprovider.h",
        "stlinkutilgdbserverprovider.cpp", "stlinkutilgdbserverprovider.h",
    ]
}
