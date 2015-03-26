import qbs 1.0
import qbs.FileInfo

QtcPlugin {
    name: "Designer"

    Depends { name: "Qt"; submodules: ["widgets", "xml", "printsupport", "designer", "designercomponents"] }
    Depends { name: "CPlusPlus" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ResourceEditor" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }

    pluginTestDepends: [
        "CppEditor",
    ]

    cpp.defines: base.concat(["CPP_ENABLED"])
    cpp.includePaths: base.concat([
        sharedSources.prefix
    ])

    Group {
        name: "General"
        files: [
            "README.txt",
            "codemodelhelpers.cpp", "codemodelhelpers.h",
            "designer.qrc",
            "designer_export.h",
            "designerconstants.h",
            "designercontext.cpp", "designercontext.h",
            "editordata.h",
            "editorwidget.cpp", "editorwidget.h",
            "formeditorfactory.cpp", "formeditorfactory.h",
            "formeditorplugin.cpp", "formeditorplugin.h",
            "formeditorstack.cpp", "formeditorstack.h",
            "formeditorw.cpp", "formeditorw.h",
            "formtemplatewizardpage.cpp", "formtemplatewizardpage.h",
            "formwindoweditor.cpp", "formwindoweditor.h",
            "formwindowfile.cpp", "formwindowfile.h",
            "qtcreatorintegration.cpp", "qtcreatorintegration.h",
            "qtdesignerformclasscodegenerator.cpp", "qtdesignerformclasscodegenerator.h",
            "resourcehandler.cpp", "resourcehandler.h",
            "settingsmanager.cpp", "settingsmanager.h",
            "settingspage.cpp", "settingspage.h",
        ]
    }

    Group {
        name: "Shared Sources"
        id: sharedSources
        prefix: project.sharedSourcesDir + "/designerintegrationv2/"
        files: [
            "formresizer.cpp", "formresizer.h",
            "sizehandlerect.cpp", "sizehandlerect.h",
            "widgethost.cpp", "widgethost.h",
            "widgethostconstants.h",
        ]
    }

    Group {
        name: "cpp"
        prefix: "cpp/"
        files: [
            "formclasswizard.cpp", "formclasswizard.h",
            "formclasswizarddialog.cpp", "formclasswizarddialog.h",
            "formclasswizardpage.cpp", "formclasswizardpage.h", "formclasswizardpage.ui",
            "formclasswizardparameters.cpp", "formclasswizardparameters.h",
        ]
    }

    Group {
        name: "Tests"
        condition: project.testsEnabled
        files: [ "gotoslot_test.cpp" ]

        cpp.defines: outer.concat(['SRCDIR="' + FileInfo.path(filePath) + '"'])
    }

}
