import qbs 1.0

Project {
    name: "ProjectExplorer"

    QtcDevHeaders { }

    QtcPlugin {
        Depends { name: "Qt"; submodules: ["widgets", "xml", "network", "qml"] }
        Depends { name: "Aggregation" }
        Depends { name: "QtcSsh" }
        Depends { name: "Utils" }

        Depends { name: "Core" }
        Depends { name: "TextEditor" }
        Depends { name: "app_version_header" }

        cpp.defines: base.concat("QTC_CPU=X86Architecture")

        Group {
            name: "General"
            files: [
                "abi.cpp", "abi.h",
                "abiwidget.cpp", "abiwidget.h",
                "abstractprocessstep.cpp", "abstractprocessstep.h",
                "allprojectsfilter.cpp", "allprojectsfilter.h",
                "allprojectsfind.cpp", "allprojectsfind.h",
                "ansifilterparser.cpp", "ansifilterparser.h",
                "applicationlauncher.cpp", "applicationlauncher.h",
                "appoutputpane.cpp", "appoutputpane.h",
                "baseprojectwizarddialog.cpp", "baseprojectwizarddialog.h",
                "buildconfiguration.cpp", "buildconfiguration.h",
                "buildenvironmentwidget.cpp", "buildenvironmentwidget.h",
                "buildinfo.cpp", "buildinfo.h",
                "buildmanager.cpp", "buildmanager.h",
                "buildprogress.cpp", "buildprogress.h",
                "buildsettingspropertiespage.cpp", "buildsettingspropertiespage.h",
                "buildstep.cpp", "buildstep.h",
                "buildsteplist.cpp", "buildsteplist.h",
                "buildstepspage.cpp", "buildstepspage.h",
                "buildtargetinfo.h",
                "clangparser.cpp", "clangparser.h",
                "codestylesettingspropertiespage.cpp", "codestylesettingspropertiespage.h", "codestylesettingspropertiespage.ui",
                "compileoutputwindow.cpp", "compileoutputwindow.h",
                "configtaskhandler.cpp", "configtaskhandler.h",
                "copytaskhandler.cpp", "copytaskhandler.h",
                "currentprojectfilter.cpp", "currentprojectfilter.h",
                "currentprojectfind.cpp", "currentprojectfind.h",
                "customexecutableconfigurationwidget.cpp", "customexecutableconfigurationwidget.h",
                "customexecutablerunconfiguration.cpp", "customexecutablerunconfiguration.h",
                "customparser.cpp", "customparser.h",
                "customparserconfigdialog.cpp", "customparserconfigdialog.h", "customparserconfigdialog.ui",
                "customtoolchain.cpp", "customtoolchain.h",
                "dependenciespanel.cpp", "dependenciespanel.h",
                "deployablefile.cpp", "deployablefile.h",
                "deployconfiguration.cpp", "deployconfiguration.h",
                "deploymentdata.h",
                "deploymentdataview.cpp",
                "deploymentdataview.h",
                "deploymentdataview.ui",
                "deploymentdatamodel.cpp",
                "deploymentdatamodel.h",
                "editorconfiguration.cpp", "editorconfiguration.h",
                "editorsettingspropertiespage.cpp", "editorsettingspropertiespage.h", "editorsettingspropertiespage.ui",
                "environmentaspect.cpp", "environmentaspect.h",
                "environmentaspectwidget.cpp", "environmentaspectwidget.h",
                "environmentwidget.cpp", "environmentwidget.h",
                "expanddata.cpp", "expanddata.h",
                "extracompiler.cpp", "extracompiler.h",
                "foldernavigationwidget.cpp", "foldernavigationwidget.h",
                "gccparser.cpp", "gccparser.h",
                "gcctoolchain.cpp", "gcctoolchain.h",
                "gcctoolchainfactories.h",
                "gnumakeparser.cpp", "gnumakeparser.h",
                "headerpath.h",
                "importwidget.cpp", "importwidget.h",
                "ioutputparser.cpp", "ioutputparser.h",
                "ipotentialkit.h",
                "itaskhandler.h",
                "kit.cpp", "kit.h",
                "kitchooser.cpp", "kitchooser.h",
                "kitconfigwidget.cpp", "kitconfigwidget.h",
                "kitfeatureprovider.h",
                "kitinformation.cpp", "kitinformation.h",
                "kitinformationconfigwidget.cpp", "kitinformationconfigwidget.h",
                "kitmanager.cpp", "kitmanager.h",
                "kitmanagerconfigwidget.cpp", "kitmanagerconfigwidget.h",
                "kitmodel.cpp", "kitmodel.h",
                "kitoptionspage.cpp", "kitoptionspage.h",
                "ldparser.cpp", "ldparser.h",
                "linuxiccparser.cpp", "linuxiccparser.h",
                "localenvironmentaspect.cpp", "localenvironmentaspect.h",
                "miniprojecttargetselector.cpp", "miniprojecttargetselector.h",
                "msvcparser.cpp", "msvcparser.h",
                "namedwidget.cpp", "namedwidget.h",
                "osparser.cpp", "osparser.h",
                "panelswidget.cpp", "panelswidget.h",
                "processparameters.cpp", "processparameters.h",
                "processstep.cpp", "processstep.h", "processstep.ui",
                "project.cpp", "project.h",
                "projectconfiguration.cpp", "projectconfiguration.h",
                "projectconfigurationmodel.cpp", "projectconfigurationmodel.h",
                "projectexplorer.cpp", "projectexplorer.h",
                "projectexplorer.qrc",
                "projectexplorer_export.h",
                "projectexplorer_global.h",
                "projectexplorerconstants.h",
                "projectexplorericons.h", "projectexplorericons.cpp",
                "projectexplorersettings.h",
                "projectexplorersettingspage.cpp", "projectexplorersettingspage.h", "projectexplorersettingspage.ui",
                "projectfilewizardextension.cpp", "projectfilewizardextension.h",
                "projectimporter.cpp", "projectimporter.h",
                "projectmacro.cpp", "projectmacro.h",
                "projectmacroexpander.cpp", "projectmacroexpander.h",
                "projectmanager.h",
                "projectmodels.cpp", "projectmodels.h",
                "projectnodes.cpp", "projectnodes.h",
                "projectpanelfactory.cpp", "projectpanelfactory.h",
                "projecttree.cpp",
                "projecttree.h",
                "projecttreewidget.cpp", "projecttreewidget.h",
                "projectwindow.cpp", "projectwindow.h",
                "projectwizardpage.cpp", "projectwizardpage.h", "projectwizardpage.ui",
                "removetaskhandler.cpp", "removetaskhandler.h",
                "runnables.cpp", "runnables.h",
                "runconfiguration.cpp", "runconfiguration.h",
                "runconfigurationaspects.cpp", "runconfigurationaspects.h",
                "runsettingspropertiespage.cpp", "runsettingspropertiespage.h",
                "selectablefilesmodel.cpp", "selectablefilesmodel.h",
                "session.cpp", "session.h",
                "sessionmodel.cpp", "sessionmodel.h",
                "sessionview.cpp", "sessionview.h",
                "sessiondialog.cpp", "sessiondialog.h", "sessiondialog.ui",
                "settingsaccessor.cpp", "settingsaccessor.h",
                "showineditortaskhandler.cpp", "showineditortaskhandler.h",
                "showoutputtaskhandler.cpp", "showoutputtaskhandler.h",
                "subscription.cpp", "subscription.h",
                "target.cpp", "target.h",
                "targetsettingspanel.cpp", "targetsettingspanel.h",
                "targetsetuppage.cpp", "targetsetuppage.h",
                "targetsetupwidget.cpp", "targetsetupwidget.h",
                "task.cpp", "task.h",
                "taskhub.cpp", "taskhub.h",
                "taskmodel.cpp", "taskmodel.h",
                "taskwindow.cpp", "taskwindow.h",
                "toolchain.cpp", "toolchain.h",
                "toolchainconfigwidget.cpp", "toolchainconfigwidget.h",
                "toolchainmanager.cpp", "toolchainmanager.h",
                "toolchainoptionspage.cpp", "toolchainoptionspage.h",
                "vcsannotatetaskhandler.cpp", "vcsannotatetaskhandler.h",
                "waitforstopdialog.cpp", "waitforstopdialog.h",
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
                "jsonwizardscannergenerator.cpp", "jsonwizardscannergenerator.h"
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
                "devicefactoryselectiondialog.cpp", "devicefactoryselectiondialog.h", "devicefactoryselectiondialog.ui",
                "devicemanager.cpp", "devicemanager.h",
                "devicemanagermodel.cpp", "devicemanagermodel.h",
                "deviceprocess.cpp", "deviceprocess.h",
                "deviceprocessesdialog.cpp", "deviceprocessesdialog.h",
                "deviceprocesslist.cpp", "deviceprocesslist.h",
                "devicesettingspage.cpp", "devicesettingspage.h",
                "devicesettingswidget.cpp", "devicesettingswidget.h", "devicesettingswidget.ui",
                "devicetestdialog.cpp", "devicetestdialog.h", "devicetestdialog.ui",
                "deviceusedportsgatherer.cpp", "deviceusedportsgatherer.h",
                "idevice.cpp", "idevice.h",
                "idevicefactory.cpp", "idevicefactory.h",
                "idevicewidget.h",
                "desktopdeviceprocess.cpp", "desktopdeviceprocess.h",
                "localprocesslist.cpp", "localprocesslist.h",
                "sshdeviceprocess.cpp", "sshdeviceprocess.h",
                "sshdeviceprocesslist.cpp", "sshdeviceprocesslist.h",
                "desktopprocesssignaloperation.cpp", "desktopprocesssignaloperation.h",
                "desktopdeviceconfigurationwidget.cpp", "desktopdeviceconfigurationwidget.h", "desktopdeviceconfigurationwidget.ui"
            ]
        }

        Group {
            name: "Images"
            prefix: "images/"
            files: ["*.png"]
        }

        Group {
            name: "WindowsToolChains"
            condition: qbs.targetOS.contains("windows") || qtc.testsEnabled
            files: [
                "abstractmsvctoolchain.cpp",
                "abstractmsvctoolchain.h",
                "msvctoolchain.cpp",
                "msvctoolchain.h",
                "windebuginterface.cpp",
                "windebuginterface.h",
            ]
        }

        Group {
            name: "Tests"
            condition: qtc.testsEnabled
            files: ["outputparser_test.h", "outputparser_test.cpp"]
        }

        Export {
            Depends { name: "Qt.network" }
        }
    }
}
