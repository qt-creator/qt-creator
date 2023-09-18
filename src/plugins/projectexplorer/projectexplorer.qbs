QtcPlugin {
    name: "ProjectExplorer"

    Depends { name: "Qt"; submodules: ["widgets", "xml", "network", "qml"] }
    Depends { name: "Aggregation" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    Depends { name: "libclang"; required: false }
    Depends { name: "clang_defines" }

    pluginTestDepends: ["GenericProjectManager"]

    Group {
        name: "General"
        files: [
            "abi.cpp", "abi.h",
            "abiwidget.cpp", "abiwidget.h",
            "abstractprocessstep.cpp", "abstractprocessstep.h",
            "addrunconfigdialog.cpp", "addrunconfigdialog.h",
            "allprojectsfilter.cpp", "allprojectsfilter.h",
            "allprojectsfind.cpp", "allprojectsfind.h",
            "appoutputpane.cpp", "appoutputpane.h",
            "baseprojectwizarddialog.cpp", "baseprojectwizarddialog.h",
            "buildaspects.cpp", "buildaspects.h",
            "buildconfiguration.cpp", "buildconfiguration.h",
            "buildinfo.cpp", "buildinfo.h",
            "buildmanager.cpp", "buildmanager.h",
            "buildprogress.cpp", "buildprogress.h",
            "buildpropertiessettings.cpp", "buildpropertiessettings.h",
            "buildsettingspropertiespage.cpp", "buildsettingspropertiespage.h",
            "buildstep.cpp", "buildstep.h",
            "buildsteplist.cpp", "buildsteplist.h",
            "buildstepspage.cpp", "buildstepspage.h",
            "buildsystem.cpp", "buildsystem.h",
            "buildtargetinfo.h",
            "buildtargettype.h",
            "clangparser.cpp", "clangparser.h",
            "codestylesettingspropertiespage.cpp", "codestylesettingspropertiespage.h",
            "compileoutputwindow.cpp", "compileoutputwindow.h",
            "configtaskhandler.cpp", "configtaskhandler.h",
            "copystep.cpp", "copystep.h",
            "copytaskhandler.cpp", "copytaskhandler.h",
            "currentprojectfilter.cpp", "currentprojectfilter.h",
            "currentprojectfind.cpp", "currentprojectfind.h",
            "customexecutablerunconfiguration.cpp", "customexecutablerunconfiguration.h",
            "customparser.cpp", "customparser.h",
            "customparserconfigdialog.cpp", "customparserconfigdialog.h",
            "customparserssettingspage.cpp", "customparserssettingspage.h",
            "customtoolchain.cpp", "customtoolchain.h",
            "dependenciespanel.cpp", "dependenciespanel.h",
            "deployablefile.cpp", "deployablefile.h",
            "deployconfiguration.cpp", "deployconfiguration.h",
            "deploymentdata.cpp",
            "deploymentdata.h",
            "deploymentdataview.cpp",
            "deploymentdataview.h",
            "desktoprunconfiguration.cpp", "desktoprunconfiguration.h",
            "editorconfiguration.cpp", "editorconfiguration.h",
            "editorsettingspropertiespage.cpp", "editorsettingspropertiespage.h",
            "environmentaspect.cpp", "environmentaspect.h",
            "environmentaspectwidget.cpp", "environmentaspectwidget.h",
            "environmentwidget.cpp", "environmentwidget.h",
            "expanddata.cpp", "expanddata.h",
            "extraabi.cpp", "extraabi.h",
            "extracompiler.cpp", "extracompiler.h",
            "fileinsessionfinder.cpp", "fileinsessionfinder.h",
            "filesinallprojectsfind.cpp", "filesinallprojectsfind.h",
            "filterkitaspectsdialog.cpp", "filterkitaspectsdialog.h",
            "gccparser.cpp", "gccparser.h",
            "gcctoolchain.cpp", "gcctoolchain.h",
            "gnumakeparser.cpp", "gnumakeparser.h",
            "headerpath.h",
            "importwidget.cpp", "importwidget.h",
            "ioutputparser.cpp", "ioutputparser.h",
            "ipotentialkit.h",
            "itaskhandler.h",
            "kit.cpp", "kit.h",
            "kitaspects.cpp", "kitaspects.h",
            "kitchooser.cpp", "kitchooser.h",
            "kitfeatureprovider.h",
            "kitmanager.cpp", "kitmanager.h",
            "kitmanagerconfigwidget.cpp", "kitmanagerconfigwidget.h",
            "kitoptionspage.cpp", "kitoptionspage.h",
            "ldparser.cpp", "ldparser.h",
            "lldparser.cpp", "lldparser.h",
            "linuxiccparser.cpp", "linuxiccparser.h",
            "makestep.cpp", "makestep.h",
            "miniprojecttargetselector.cpp", "miniprojecttargetselector.h",
            "msvcparser.cpp", "msvcparser.h",
            "namedwidget.cpp", "namedwidget.h",
            "osparser.cpp", "osparser.h",
            "panelswidget.cpp", "panelswidget.h",
            "parseissuesdialog.cpp", "parseissuesdialog.h",
            "processparameters.cpp", "processparameters.h",
            "processstep.cpp", "processstep.h",
            "project.cpp", "project.h",
            "projectcommentssettings.cpp", "projectcommentssettings.h",
            "projectconfiguration.cpp", "projectconfiguration.h",
            "projectconfigurationmodel.cpp", "projectconfigurationmodel.h",
            "projectexplorer.cpp", "projectexplorer.h",
            "projectexplorer.qrc",
            "projectexplorer_export.h",
            "projectexplorerconstants.cpp",
            "projectexplorerconstants.h",
            "projectexplorericons.h", "projectexplorericons.cpp",
            "projectexplorersettings.h", "projectexplorersettings.cpp",
            "projectexplorertr.h",
            "projectfilewizardextension.cpp", "projectfilewizardextension.h",
            "projectimporter.cpp", "projectimporter.h",
            "projectmacro.cpp", "projectmacro.h",
            "projectmanager.cpp", "projectmanager.h",
            "projectmodels.cpp", "projectmodels.h",
            "projectnodes.cpp", "projectnodes.h",
            "projectpanelfactory.cpp", "projectpanelfactory.h",
            "projectsettingswidget.cpp", "projectsettingswidget.h",
            "projecttree.cpp",
            "projecttree.h",
            "projecttreewidget.cpp", "projecttreewidget.h",
            "projectwindow.cpp", "projectwindow.h",
            "projectwizardpage.cpp", "projectwizardpage.h",
            "rawprojectpart.cpp", "rawprojectpart.h",
            "removetaskhandler.cpp", "removetaskhandler.h",
            "runconfiguration.cpp", "runconfiguration.h",
            "runcontrol.cpp", "runcontrol.h",
            "runconfigurationaspects.cpp", "runconfigurationaspects.h",
            "runsettingspropertiespage.cpp", "runsettingspropertiespage.h",
            "sanitizerparser.cpp", "sanitizerparser.h",
            "selectablefilesmodel.cpp", "selectablefilesmodel.h",
            "showineditortaskhandler.cpp", "showineditortaskhandler.h",
            "showoutputtaskhandler.cpp", "showoutputtaskhandler.h",
            "simpleprojectwizard.cpp", "simpleprojectwizard.h",
            "target.cpp", "target.h",
            "targetsettingspanel.cpp", "targetsettingspanel.h",
            "targetsetuppage.cpp", "targetsetuppage.h",
            "targetsetupwidget.cpp", "targetsetupwidget.h",
            "task.cpp", "task.h",
            "taskfile.cpp", "taskfile.h",
            "taskhub.cpp", "taskhub.h",
            "taskmodel.cpp", "taskmodel.h",
            "taskwindow.cpp", "taskwindow.h",
            "toolchain.cpp", "toolchain.h",
            "toolchaincache.h",
            "toolchainconfigwidget.cpp", "toolchainconfigwidget.h",
            "toolchainmanager.cpp", "toolchainmanager.h",
            "toolchainoptionspage.cpp", "toolchainoptionspage.h",
            "toolchainsettingsaccessor.cpp", "toolchainsettingsaccessor.h",
            "treescanner.cpp", "treescanner.h",
            "userfileaccessor.cpp", "userfileaccessor.h",
            "vcsannotatetaskhandler.cpp", "vcsannotatetaskhandler.h",
            "waitforstopdialog.cpp", "waitforstopdialog.h",
            "windebuginterface.cpp", "windebuginterface.h",
            "xcodebuildparser.cpp", "xcodebuildparser.h"
        ]
    }

    Group {
        name: "Project Welcome Page"
        files: [
            "projectwelcomepage.cpp",
            "projectwelcomepage.h"
        ]
    }

    Group {
        name: "JsonWizard"
        prefix: "jsonwizard/"
        files: [
            "jsonfieldpage.cpp", "jsonfieldpage_p.h", "jsonfieldpage.h",
            "jsonfilepage.cpp", "jsonfilepage.h",
            "jsonkitspage.cpp", "jsonkitspage.h",
            "jsonprojectpage.cpp", "jsonprojectpage.h",
            "jsonsummarypage.cpp", "jsonsummarypage.h",
            "jsonwizard.cpp", "jsonwizard.h",
            "jsonwizardfactory.cpp", "jsonwizardfactory.h",
            "jsonwizardfilegenerator.cpp", "jsonwizardfilegenerator.h",
            "jsonwizardgeneratorfactory.cpp", "jsonwizardgeneratorfactory.h",
            "jsonwizardpagefactory.cpp", "jsonwizardpagefactory.h",
            "jsonwizardpagefactory_p.cpp", "jsonwizardpagefactory_p.h",
            "jsonwizardscannergenerator.cpp", "jsonwizardscannergenerator.h",
            "wizarddebug.h"
        ]
    }

    Group {
        name: "CustomWizard"
        prefix: "customwizard/"
        files: [
            "customwizard.cpp", "customwizard.h",
            "customwizardpage.cpp", "customwizardpage.h",
            "customwizardparameters.cpp", "customwizardparameters.h",
            "customwizardscriptgenerator.cpp", "customwizardscriptgenerator.h"
        ]
    }

    Group {
        name: "Device Support"
        prefix: "devicesupport/"
        files: [
            "desktopdevice.cpp", "desktopdevice.h",
            "desktopdevicefactory.cpp", "desktopdevicefactory.h",
            "devicecheckbuildstep.cpp", "devicecheckbuildstep.h",
            "devicefactoryselectiondialog.cpp", "devicefactoryselectiondialog.h",
            "devicemanager.cpp", "devicemanager.h",
            "devicemanagermodel.cpp", "devicemanagermodel.h",
            "deviceprocessesdialog.cpp", "deviceprocessesdialog.h",
            "devicesettingspage.cpp", "devicesettingspage.h",
            "devicetestdialog.cpp", "devicetestdialog.h",
            "deviceusedportsgatherer.cpp", "deviceusedportsgatherer.h",
            "filetransfer.cpp", "filetransfer.h",
            "filetransferinterface.h",
            "idevice.cpp", "idevice.h",
            "idevicefactory.cpp", "idevicefactory.h",
            "idevicefwd.h",
            "idevicewidget.h",
            "processlist.cpp", "processlist.h",
            "sshparameters.cpp", "sshparameters.h",
            "sshsettings.cpp", "sshsettings.h",
            "sshsettingspage.cpp", "sshsettingspage.h",
            "desktopprocesssignaloperation.cpp", "desktopprocesssignaloperation.h"
        ]
    }

    Group {
        name: "Images"
        prefix: "images/"
        files: ["*.png"]
    }

    Group {
        name: "WindowsToolChains"
        condition: qbs.targetOS.contains("windows") || qtc.withPluginTests
        files: [
            "msvctoolchain.cpp",
            "msvctoolchain.h",
        ]
    }

    QtcTestFiles {
        files: ["outputparser_test.h", "outputparser_test.cpp"]
    }

    Group {
        name: "Test resources"
        condition: qtc.withPluginTests
        files: ["testdata/**"]
        fileTags: ["qt.core.resource_data"]
        Qt.core.resourcePrefix: "/projectexplorer"
        Qt.core.resourceSourceBase: path
    }

    Export {
        Depends { name: "Qt.network" }
    }
}
