import qbs

QtcPlugin {
    name: "BareMetal"

    Depends { name: "Qt"; submodules: ["network", "widgets"]; }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }

    Group {
        name: "General"
        files: [
            "baremetal.qrc",
            "baremetalconstants.h",
            "baremetalcustomrunconfiguration.cpp", "baremetalcustomrunconfiguration.h",
            "baremetaldebugsupport.cpp", "baremetaldebugsupport.h",
            "baremetaldevice.cpp", "baremetaldevice.h",
            "baremetaldeviceconfigurationwidget.cpp", "baremetaldeviceconfigurationwidget.h",
            "baremetaldeviceconfigurationwizard.cpp", "baremetaldeviceconfigurationwizard.h",
            "baremetaldeviceconfigurationwizardpages.cpp", "baremetaldeviceconfigurationwizardpages.h",
            "baremetalplugin.cpp", "baremetalplugin.h",
            "baremetalrunconfiguration.cpp", "baremetalrunconfiguration.h",
            "debugserverproviderchooser.cpp", "debugserverproviderchooser.h",
            "debugserverprovidermanager.cpp", "debugserverprovidermanager.h",
            "debugserverproviderssettingspage.cpp", "debugserverproviderssettingspage.h",
            "iarewparser.cpp", "iarewparser.h",
            "iarewtoolchain.cpp", "iarewtoolchain.h",
            "idebugserverprovider.cpp", "idebugserverprovider.h",
            "keilparser.cpp", "keilparser.h",
            "keiltoolchain.cpp", "keiltoolchain.h",
            "sdccparser.cpp", "sdccparser.h",
            "sdcctoolchain.cpp", "sdcctoolchain.h",
        ]
    }

    Group {
        name: "GDB Servers"
        prefix: "debugservers/gdb/"
        files: [
            "gdbserverprovider.cpp", "gdbserverprovider.h",
            "openocdgdbserverprovider.cpp", "openocdgdbserverprovider.h",
            "stlinkutilgdbserverprovider.cpp", "stlinkutilgdbserverprovider.h",
            "jlinkgdbserverprovider.cpp", "jlinkgdbserverprovider.h",
            "eblinkgdbserverprovider.cpp", "eblinkgdbserverprovider.h",
        ]
    }
}
