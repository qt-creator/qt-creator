add_qtc_plugin(CppEditor
  DEPENDS Qt::Network Qt::Xml
  PUBLIC_DEPENDS CPlusPlus Qt::Widgets
  PLUGIN_DEPENDS Core ProjectExplorer TextEditor
  PLUGIN_TEST_DEPENDS QbsProjectManager QmakeProjectManager
  SOURCES
    abstracteditorsupport.cpp abstracteditorsupport.h
    baseeditordocumentparser.cpp baseeditordocumentparser.h
    baseeditordocumentprocessor.cpp baseeditordocumentprocessor.h
    builtincursorinfo.cpp builtincursorinfo.h
    builtineditordocumentparser.cpp builtineditordocumentparser.h
    builtineditordocumentprocessor.cpp builtineditordocumentprocessor.h
    clangdiagnosticconfig.cpp clangdiagnosticconfig.h
    clangdiagnosticconfigsmodel.cpp clangdiagnosticconfigsmodel.h
    clangdiagnosticconfigsselectionwidget.cpp clangdiagnosticconfigsselectionwidget.h
    clangdiagnosticconfigswidget.cpp clangdiagnosticconfigswidget.h
    compileroptionsbuilder.cpp compileroptionsbuilder.h
    cppautocompleter.cpp cppautocompleter.h
    cppbuiltinmodelmanagersupport.cpp cppbuiltinmodelmanagersupport.h
    cppcanonicalsymbol.cpp cppcanonicalsymbol.h
    cppchecksymbols.cpp cppchecksymbols.h
    cppcodeformatter.cpp cppcodeformatter.h
    cppcodemodelinspectordialog.cpp cppcodemodelinspectordialog.h
    cppcodemodelinspectordumper.cpp cppcodemodelinspectordumper.h
    cppcodemodelsettings.cpp cppcodemodelsettings.h
    cppcodemodelsettingspage.cpp cppcodemodelsettingspage.h
    cppcodestylepreferences.cpp cppcodestylepreferences.h
    cppcodestylepreferencesfactory.cpp cppcodestylepreferencesfactory.h
    cppcodestylesettings.cpp cppcodestylesettings.h
    cppcodestylesettingspage.cpp cppcodestylesettingspage.h
    cppcodestylesnippets.h
    cppcompletionassist.cpp cppcompletionassist.h
    cppcompletionassistprocessor.cpp cppcompletionassistprocessor.h
    cppcompletionassistprovider.cpp cppcompletionassistprovider.h
    cppcursorinfo.h
    cppdoxygen.cpp cppdoxygen.h
    cppeditor.qrc
    cppeditor_global.h
    cppeditortr.h
    cppeditorconstants.h
    cppeditordocument.cpp cppeditordocument.h
    cppeditoroutline.cpp cppeditoroutline.h
    cppeditorplugin.cpp cppeditorplugin.h
    cppeditorwidget.cpp cppeditorwidget.h
    cppelementevaluator.cpp cppelementevaluator.h
    cppfileiterationorder.cpp cppfileiterationorder.h
    cppfilesettingspage.cpp cppfilesettingspage.h
    cppfindreferences.cpp cppfindreferences.h
    cppfollowsymbolundercursor.cpp cppfollowsymbolundercursor.h
    cppfunctiondecldeflink.cpp cppfunctiondecldeflink.h
    cpphighlighter.cpp cpphighlighter.h
    cppincludehierarchy.cpp cppincludehierarchy.h
    cppincludesfilter.cpp cppincludesfilter.h
    cppindexingsupport.cpp cppindexingsupport.h
    cppinsertvirtualmethods.cpp cppinsertvirtualmethods.h
    cpplocalrenaming.cpp cpplocalrenaming.h
    cpplocalsymbols.cpp cpplocalsymbols.h
    cpplocatordata.cpp cpplocatordata.h
    cpplocatorfilter.cpp cpplocatorfilter.h
    cppmodelmanager.cpp cppmodelmanager.h
    cppmodelmanagersupport.cpp cppmodelmanagersupport.h
    cppoutline.cpp cppoutline.h
    cppoutlinemodel.cpp cppoutlinemodel.h
    cppparsecontext.cpp cppparsecontext.h
    cpppointerdeclarationformatter.cpp cpppointerdeclarationformatter.h
    cpppreprocessordialog.cpp cpppreprocessordialog.h
    cppprojectfile.cpp cppprojectfile.h
    cppprojectfilecategorizer.cpp cppprojectfilecategorizer.h
    cppprojectinfogenerator.cpp cppprojectinfogenerator.h
    cppprojectpartchooser.cpp cppprojectpartchooser.h
    cppprojectupdater.cpp cppprojectupdater.h
    cppprojectupdaterinterface.h
    cppqtstyleindenter.cpp cppqtstyleindenter.h
    cppquickfix.cpp cppquickfix.h
    cppquickfixassistant.cpp cppquickfixassistant.h
    cppquickfixes.cpp cppquickfixes.h
    cppquickfixprojectsettings.cpp cppquickfixprojectsettings.h
    cppquickfixprojectsettingswidget.cpp cppquickfixprojectsettingswidget.h
    cppquickfixsettings.cpp cppquickfixsettings.h
    cppquickfixsettingspage.cpp cppquickfixsettingspage.h
    cppquickfixsettingswidget.cpp cppquickfixsettingswidget.h
    cpprefactoringchanges.cpp cpprefactoringchanges.h
    cppselectionchanger.cpp cppselectionchanger.h
    cppsemanticinfo.h
    cppsemanticinfoupdater.cpp cppsemanticinfoupdater.h
    cppsourceprocessor.cpp cppsourceprocessor.h
    cpptoolsjsextension.cpp cpptoolsjsextension.h
    cpptoolsreuse.cpp cpptoolsreuse.h
    cpptoolssettings.cpp cpptoolssettings.h
    cpptypehierarchy.cpp cpptypehierarchy.h
    cppuseselectionsupdater.cpp cppuseselectionsupdater.h
    cppvirtualfunctionassistprovider.cpp cppvirtualfunctionassistprovider.h
    cppvirtualfunctionproposalitem.cpp cppvirtualfunctionproposalitem.h
    cppworkingcopy.cpp cppworkingcopy.h
    cursorineditor.h
    doxygengenerator.cpp doxygengenerator.h
    editordocumenthandle.cpp editordocumenthandle.h
    functionutils.cpp functionutils.h
    generatedcodemodelsupport.cpp generatedcodemodelsupport.h
    headerpathfilter.cpp headerpathfilter.h
    includeutils.cpp includeutils.h
    indexitem.cpp indexitem.h
    insertionpointlocator.cpp insertionpointlocator.h
    projectinfo.cpp projectinfo.h
    projectpart.cpp projectpart.h
    resourcepreviewhoverhandler.cpp resourcepreviewhoverhandler.h
    searchsymbols.cpp searchsymbols.h
    semantichighlighter.cpp semantichighlighter.h
    symbolfinder.cpp symbolfinder.h
    symbolsfindfilter.cpp symbolsfindfilter.h
    typehierarchybuilder.cpp typehierarchybuilder.h
    wrappablelineedit.cpp wrappablelineedit.h
   EXPLICIT_MOC
     cppquickfixsettingswidget.h
     cppquickfixprojectsettingswidget.h
)

extend_qtc_plugin(CppEditor
  CONDITION WITH_TESTS
  SOURCES
    compileroptionsbuilder_test.cpp compileroptionsbuilder_test.h
    cppcodegen_test.cpp cppcodegen_test.h
    cppcompletion_test.cpp cppcompletion_test.h
    cppdoxygen_test.cpp cppdoxygen_test.h
    cppheadersource_test.cpp cppheadersource_test.h
    cppincludehierarchy_test.cpp cppincludehierarchy_test.h
    cpplocalsymbols_test.cpp cpplocalsymbols_test.h
    cpplocatorfilter_test.cpp cpplocatorfilter_test.h
    cppmodelmanager_test.cpp cppmodelmanager_test.h
    cpppointerdeclarationformatter_test.cpp cpppointerdeclarationformatter_test.h
    cppquickfix_test.cpp cppquickfix_test.h
    cpprenaming_test.cpp cpprenaming_test.h
    cppsourceprocessertesthelper.cpp cppsourceprocessertesthelper.h
    cppsourceprocessor_test.cpp cppsourceprocessor_test.h
    cpptoolstestcase.cpp cpptoolstestcase.h
    cppuseselections_test.cpp cppuseselections_test.h
    fileandtokenactions_test.cpp fileandtokenactions_test.h
    followsymbol_switchmethoddecldef_test.cpp followsymbol_switchmethoddecldef_test.h
    modelmanagertesthelper.cpp modelmanagertesthelper.h
    projectinfo_test.cpp projectinfo_test.h
    symbolsearcher_test.cpp symbolsearcher_test.h
    typehierarchybuilder_test.cpp typehierarchybuilder_test.h
  EXPLICIT_MOC
    cppdoxygen_test.h
    cppquickfix_test.h
)
