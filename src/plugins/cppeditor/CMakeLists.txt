add_qtc_plugin(CppEditor
  DEFINES CPPEDITOR_LIBRARY
  PLUGIN_DEPENDS Core CppTools ProjectExplorer TextEditor
  PLUGIN_TEST_DEPENDS QbsProjectManager QmakeProjectManager
  SOURCES
    cppautocompleter.cpp cppautocompleter.h
    cppcodemodelinspectordialog.cpp cppcodemodelinspectordialog.h cppcodemodelinspectordialog.ui
    cppdocumentationcommenthelper.cpp cppdocumentationcommenthelper.h
    cppeditor.cpp cppeditor.h
    cppeditor.qrc
    cppeditor_global.h
    cppeditorconstants.h
    cppeditordocument.cpp cppeditordocument.h
    cppeditorenums.h
    cppeditorplugin.cpp cppeditorplugin.h
    cppeditorwidget.cpp cppeditorwidget.h
    cppfunctiondecldeflink.cpp cppfunctiondecldeflink.h
    cpphighlighter.cpp cpphighlighter.h
    cppincludehierarchy.cpp cppincludehierarchy.h
    cppinsertvirtualmethods.cpp cppinsertvirtualmethods.h
    cpplocalrenaming.cpp cpplocalrenaming.h
    cppminimizableinfobars.cpp cppminimizableinfobars.h
    cppoutline.cpp cppoutline.h
    cppparsecontext.cpp cppparsecontext.h
    cpppreprocessordialog.cpp cpppreprocessordialog.h cpppreprocessordialog.ui
    cppquickfix.cpp cppquickfix.h
    cppquickfixassistant.cpp cppquickfixassistant.h
    cppquickfixes.cpp cppquickfixes.h
    cppquickfixprojectsettings.cpp cppquickfixprojectsettings.h
    cppquickfixprojectsettingswidget.cpp cppquickfixprojectsettingswidget.h
        cppquickfixprojectsettingswidget.ui
    cppquickfixsettings.cpp cppquickfixsettings.h
    cppquickfixsettingspage.cpp cppquickfixsettingspage.h
    cppquickfixsettingswidget.cpp cppquickfixsettingswidget.h cppquickfixsettingswidget.ui
    cpptypehierarchy.cpp cpptypehierarchy.h
    cppuseselectionsupdater.cpp cppuseselectionsupdater.h
    resourcepreviewhoverhandler.cpp resourcepreviewhoverhandler.h
   EXPLICIT_MOC
     cppeditor.h
     cppquickfixsettingswidget.h
     cppquickfixprojectsettingswidget.h
)

extend_qtc_plugin(CppEditor
  CONDITION WITH_TESTS
  SOURCES
    cppdoxygen_test.cpp cppdoxygen_test.h
    cppeditortestcase.cpp cppeditortestcase.h
    cppincludehierarchy_test.cpp cppincludehierarchy_test.h
    cppquickfix_test.cpp cppquickfix_test.h
    cppuseselections_test.cpp cppuseselections_test.h
    fileandtokenactions_test.cpp fileandtokenactions_test.h
    followsymbol_switchmethoddecldef_test.cpp followsymbol_switchmethoddecldef_test.h
  EXPLICIT_MOC
    cppdoxygen_test.h
    cppquickfix_test.h
)
