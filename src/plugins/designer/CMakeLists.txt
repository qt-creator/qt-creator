# used in the .json.in
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.7.0)
  set(DESIGNER_PLUGIN_ARGUMENTS
  "\"Arguments\" : [
    {
        \"Name\" : \"-designer-qt-pluginpath\",
        \"Parameter\" : \"path\",
        \"Description\" : \"Override the default search path for Qt Designer plugins\"
    },
    {
        \"Name\" : \"-designer-pluginpath\",
        \"Parameter\" : \"path\",
        \"Description\" : \"Add a custom search path for Qt Designer plugins\"
    }
    ],")
else()
  set(DESIGNER_PLUGIN_ARGUMENTS
  "\"Arguments\" : [
    {
        \"Name\" : \"-designer-qt-pluginpath\",
        \"Parameter\" : \"path\",
        \"Description\" : \"Override the default search path for Qt Designer plugins\"
    }
    ],")
endif()

add_qtc_plugin(Designer
  PLUGIN_CLASS FormEditorPlugin
  CONDITION TARGET Qt::DesignerComponentsPrivate AND TARGET Qt::Designer
  DEPENDS designerintegrationv2
    Qt::Designer Qt::PrintSupport Qt::DesignerComponentsPrivate
  DEFINES CPP_ENABLED
  PLUGIN_DEPENDS Core CppEditor ProjectExplorer QtSupport ResourceEditor TextEditor
  PLUGIN_TEST_DEPENDS CppEditor
  SOURCES
    codemodelhelpers.cpp codemodelhelpers.h
    cpp/formclasswizard.cpp cpp/formclasswizard.h
    cpp/formclasswizarddialog.cpp cpp/formclasswizarddialog.h
    cpp/formclasswizardpage.cpp cpp/formclasswizardpage.h
    cpp/formclasswizardparameters.cpp cpp/formclasswizardparameters.h
    cpp/newclasswidget.cpp cpp/newclasswidget.h
    designer_export.h
    designerconstants.h
    designercontext.cpp designercontext.h
    designertr.h
    editordata.h
    editorwidget.cpp editorwidget.h
    formeditorfactory.cpp formeditorfactory.h
    formeditorplugin.cpp formeditorplugin.h
    formeditorstack.cpp formeditorstack.h
    formeditor.cpp formeditor.h
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
  CONDITION WITH_TESTS
  SOURCES gotoslot_test.cpp
)

if (QTC_STATIC_BUILD AND TARGET Designer)
  # version-less target Qt::Designer is an interface library that links to QtX::Designer
  get_target_property(_designer Qt::Designer INTERFACE_LINK_LIBRARIES)
  get_target_property(_designerType ${_designer} TYPE)
  if (${_designerType} STREQUAL "STATIC_LIBRARY")
    extend_qtc_target(Designer PUBLIC_DEFINES QT_DESIGNER_STATIC)
  endif()
endif()
