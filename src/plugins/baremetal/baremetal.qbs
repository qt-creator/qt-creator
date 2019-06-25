import qbs

QtcPlugin {
    name: "BareMetal"

    Depends { name: "Qt"; submodules: ["network", "widgets"]; }
    Depends { name: "QtcSsh" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }

    files: [
        "baremetal.qrc",
        "baremetalconstants.h",
        "baremetalcustomrunconfiguration.cpp", "baremetalcustomrunconfiguration.h",
        "baremetaldevice.cpp", "baremetaldevice.h",
        "baremetaldeviceconfigurationwidget.cpp", "baremetaldeviceconfigurationwidget.h",
        "baremetaldeviceconfigurationwizard.cpp", "baremetaldeviceconfigurationwizard.h",
        "baremetaldeviceconfigurationwizardpages.cpp", "baremetaldeviceconfigurationwizardpages.h",
        "baremetalplugin.cpp", "baremetalplugin.h",
        "baremetalrunconfiguration.cpp", "baremetalrunconfiguration.h",
        "baremetaldebugsupport.cpp", "baremetaldebugsupport.h",
        "gdbserverproviderprocess.cpp", "gdbserverproviderprocess.h",
        "gdbserverproviderssettingspage.cpp", "gdbserverproviderssettingspage.h",
        "gdbserverprovider.cpp", "gdbserverprovider.h",
        "gdbserverproviderchooser.cpp", "gdbserverproviderchooser.h",
        "gdbserverprovidermanager.cpp", "gdbserverprovidermanager.h",
        "openocdgdbserverprovider.cpp", "openocdgdbserverprovider.h",
        "defaultgdbserverprovider.cpp", "defaultgdbserverprovider.h",
        "stlinkutilgdbserverprovider.cpp", "stlinkutilgdbserverprovider.h",
        "iarewtoolchain.cpp", "iarewtoolchain.h",
        "keiltoolchain.cpp", "keiltoolchain.h",
        "sdcctoolchain.cpp", "sdcctoolchain.h",
        "iarewparser.cpp", "iarewparser.h",
        "keilparser.cpp", "keilparser.h",
        "sdccparser.cpp", "sdccparser.h",
    ]
}
