import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Designer"

    Depends { name: "Qt"; submodules: ["widgets", "xml", "printsupport", "designer", "designercomponents"] }
    Depends { name: "Core" }
    Depends { name: "CPlusPlus" }
    Depends { name: "CppTools" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }

    Depends { name: "cpp" }
    cpp.defines: base.concat(["CPP_ENABLED"])
    cpp.includePaths: [
        "../../libs/3rdparty",
        "cpp",
        "../../shared/designerintegrationv2",
        ".",
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "../../shared/designerintegrationv2/widgethost.h",
        "../../shared/designerintegrationv2/sizehandlerect.h",
        "../../shared/designerintegrationv2/formresizer.h",
        "../../shared/designerintegrationv2/widgethostconstants.h",
        "cpp/formclasswizardpage.h",
        "cpp/formclasswizarddialog.h",
        "cpp/formclasswizard.h",
        "cpp/formclasswizardparameters.h",
        "cpp/cppsettingspage.h",
        "formeditorplugin.h",
        "formeditorfactory.h",
        "formwindoweditor.h",
        "formwindowfile.h",
        "formwizard.h",
        "qtcreatorintegration.h",
        "designerconstants.h",
        "settingspage.h",
        "editorwidget.h",
        "formeditorw.h",
        "settingsmanager.h",
        "formtemplatewizardpage.h",
        "formwizarddialog.h",
        "codemodelhelpers.h",
        "designer_export.h",
        "designerxmleditor.h",
        "designercontext.h",
        "formeditorstack.h",
        "editordata.h",
        "resourcehandler.h",
        "qtdesignerformclasscodegenerator.h",
        "../../shared/designerintegrationv2/widgethost.cpp",
        "../../shared/designerintegrationv2/sizehandlerect.cpp",
        "../../shared/designerintegrationv2/formresizer.cpp",
        "cpp/formclasswizardpage.cpp",
        "cpp/formclasswizarddialog.cpp",
        "cpp/formclasswizard.cpp",
        "cpp/formclasswizardparameters.cpp",
        "cpp/cppsettingspage.cpp",
        "formeditorplugin.cpp",
        "formeditorfactory.cpp",
        "formwindoweditor.cpp",
        "formwindowfile.cpp",
        "formwizard.cpp",
        "qtcreatorintegration.cpp",
        "settingspage.cpp",
        "editorwidget.cpp",
        "formeditorw.cpp",
        "settingsmanager.cpp",
        "formtemplatewizardpage.cpp",
        "formwizarddialog.cpp",
        "codemodelhelpers.cpp",
        "designerxmleditor.cpp",
        "designercontext.cpp",
        "formeditorstack.cpp",
        "resourcehandler.cpp",
        "qtdesignerformclasscodegenerator.cpp",
        "cpp/formclasswizardpage.ui",
        "cpp/cppsettingspagewidget.ui",
        "designer.qrc",
        "Designer.mimetypes.xml",
        "README.txt"
    ]
}

