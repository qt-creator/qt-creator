import qbs

import QtcPlugin

QtcPlugin {
    name: "BareMetal"
    provider: "Sander"

    Depends { name: "Qt"; submodules: ["network", "widgets"]; }
    Depends { name: "QtcSsh" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }

    files: [
        "baremetalconstants.h",
        "baremetaldevice.cpp", "baremetaldevice.h",
        "baremetaldeviceconfigurationfactory.cpp", "baremetaldeviceconfigurationfactory.h",
        "baremetaldeviceconfigurationwidget.cpp", "baremetaldeviceconfigurationwidget.h",
        "baremetaldeviceconfigurationwizard.cpp", "baremetaldeviceconfigurationwizard.h",
        "baremetaldeviceconfigurationwizardpages.cpp", "baremetaldeviceconfigurationwizardpages.h",
        "baremetaldeviceconfigurationwizardsetuppage.ui",
        "baremetalgdbcommandsdeploystep.cpp", "baremetalgdbcommandsdeploystep.h",
        "baremetalplugin.cpp", "baremetalplugin.h",
        "baremetalrunconfiguration.cpp", "baremetalrunconfiguration.h",
        "baremetalrunconfigurationfactory.cpp", "baremetalrunconfigurationfactory.h",
        "baremetalrunconfigurationwidget.cpp", "baremetalrunconfigurationwidget.h", "baremetaldeviceconfigurationwidget.ui",
        "baremetalruncontrolfactory.cpp", "baremetalruncontrolfactory.h",
    ]
}
