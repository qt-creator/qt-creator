# Qt5::DesignerComponents doesn't have a target, so we need to define our own:-/
find_package(DesignerComponents)

if (WITH_TESTS)
  set(TST_COMPONENT CppEditor)
endif()

add_qtc_plugin(Designer
  CONDITION TARGET Qt5::DesignerComponents AND TARGET Qt5::Designer
  DEPENDS designerintegrationv2
    Qt5::Designer Qt5::PrintSupport Qt5::DesignerComponents
  DEFINES CPP_ENABLED
  PLUGIN_DEPENDS Core CppTools ProjectExplorer QtSupport ResourceEditor TextEditor ${TST_COMPONENT}
  SOURCES
    codemodelhelpers.cpp codemodelhelpers.h
    cpp/formclasswizard.cpp cpp/formclasswizard.h
    cpp/formclasswizarddialog.cpp cpp/formclasswizarddialog.h
    cpp/formclasswizardpage.cpp cpp/formclasswizardpage.h cpp/formclasswizardpage.ui
    cpp/formclasswizardparameters.cpp cpp/formclasswizardparameters.h
    cpp/newclasswidget.cpp cpp/newclasswidget.h cpp/newclasswidget.ui
    designer_export.h
    designerconstants.h
    designercontext.cpp designercontext.h
    editordata.h
    editorwidget.cpp editorwidget.h
    formeditorfactory.cpp formeditorfactory.h
    formeditorplugin.cpp formeditorplugin.h
    formeditorstack.cpp formeditorstack.h
    formeditorw.cpp formeditorw.h
    formtemplatewizardpage.cpp formtemplatewizardpage.h
    formwindoweditor.cpp formwindoweditor.h
    formwindowfile.cpp formwindowfile.h
    qtcreatorintegration.cpp qtcreatorintegration.h
    qtdesignerformclasscodegenerator.cpp qtdesignerformclasscodegenerator.h
    resourcehandler.cpp resourcehandler.h
    settingsmanager.cpp settingsmanager.h
    settingspage.cpp settingspage.h
)

extend_qtc_plugin(Designer
  CONDITION WITH_TESTS AND TARGET Qt5::DesignerComponents AND TARGET Qt5::Designer
  SOURCES gotoslot_test.cpp
)
