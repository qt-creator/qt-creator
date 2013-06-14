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

    cpp.defines: base.concat(["CPP_ENABLED"])
    cpp.includePaths: base.concat([
        "../../libs/3rdparty",
        "cpp",
        "../../shared/designerintegrationv2"
    ])

    files: [
        "README.txt",
        "codemodelhelpers.cpp",
        "codemodelhelpers.h",
        "designer.qrc",
        "designer_export.h",
        "designerconstants.h",
        "designercontext.cpp",
        "designercontext.h",
        "designerxmleditor.cpp",
        "designerxmleditor.h",
        "editordata.h",
        "editorwidget.cpp",
        "editorwidget.h",
        "formeditorfactory.cpp",
        "formeditorfactory.h",
        "formeditorplugin.cpp",
        "formeditorplugin.h",
        "formeditorstack.cpp",
        "formeditorstack.h",
        "formeditorw.cpp",
        "formeditorw.h",
        "formtemplatewizardpage.cpp",
        "formtemplatewizardpage.h",
        "formwindoweditor.cpp",
        "formwindoweditor.h",
        "formwindowfile.cpp",
        "formwindowfile.h",
        "formwizard.cpp",
        "formwizard.h",
        "formwizarddialog.cpp",
        "formwizarddialog.h",
        "qtcreatorintegration.cpp",
        "qtcreatorintegration.h",
        "qtdesignerformclasscodegenerator.cpp",
        "qtdesignerformclasscodegenerator.h",
        "resourcehandler.cpp",
        "resourcehandler.h",
        "settingsmanager.cpp",
        "settingsmanager.h",
        "settingspage.cpp",
        "settingspage.h",
        "../../shared/designerintegrationv2/formresizer.cpp",
        "../../shared/designerintegrationv2/formresizer.h",
        "../../shared/designerintegrationv2/sizehandlerect.cpp",
        "../../shared/designerintegrationv2/sizehandlerect.h",
        "../../shared/designerintegrationv2/widgethost.cpp",
        "../../shared/designerintegrationv2/widgethost.h",
        "../../shared/designerintegrationv2/widgethostconstants.h",
        "cpp/cppsettingspage.cpp",
        "cpp/cppsettingspage.h",
        "cpp/cppsettingspagewidget.ui",
        "cpp/formclasswizard.cpp",
        "cpp/formclasswizard.h",
        "cpp/formclasswizarddialog.cpp",
        "cpp/formclasswizarddialog.h",
        "cpp/formclasswizardpage.cpp",
        "cpp/formclasswizardpage.h",
        "cpp/formclasswizardpage.ui",
        "cpp/formclasswizardparameters.cpp",
        "cpp/formclasswizardparameters.h",
    ]
}
