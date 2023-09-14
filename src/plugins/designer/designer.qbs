import qbs 1.0
import qbs.FileInfo

QtcPlugin {
    name: "Designer"

    Depends {
        name: "Qt"
        submodules: ["widgets", "xml", "printsupport", "designer", "designercomponents-private"]
    }
    Depends { name: "CPlusPlus" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "CppEditor" }
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

    pluginjson.replacements: ({"DESIGNER_PLUGIN_ARGUMENTS":
    "\"Arguments\" : [\n\
        {\n\
            \"Name\" : \"-designer-qt-pluginpath\",\n\
            \"Parameter\" : \"path\",\n\
            \"Description\" : \"Override the default search path for Qt Designer plugins\"\n\
        },\n\
        {\n\
            \"Name\" : \"-designer-pluginpath\",\n\
            \"Parameter\" : \"path\",\n\
            \"Description\" : \"Add a custom search path for Qt Designer plugins\"\n\
        }\n\
    ],"})

    Group {
        name: "General"
        files: [
            "README.txt",
            "codemodelhelpers.cpp", "codemodelhelpers.h",
            "designer_export.h",
            "designerconstants.h",
            "designercontext.cpp", "designercontext.h",
            "designertr.h",
            "editordata.h",
            "editorwidget.cpp", "editorwidget.h",
            "formeditorfactory.cpp", "formeditorfactory.h",
            "formeditorplugin.cpp", "formeditorplugin.h",
            "formeditorstack.cpp", "formeditorstack.h",
            "formeditor.cpp", "formeditor.h",
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
            "formclasswizardpage.cpp", "formclasswizardpage.h",
            "formclasswizardparameters.cpp", "formclasswizardparameters.h",
            "newclasswidget.cpp", "newclasswidget.h",
        ]
    }

    QtcTestFiles {
        files: [ "gotoslot_test.cpp" ]

        cpp.defines: outer.concat(['SRCDIR="' + FileInfo.path(filePath) + '"'])
    }

}
