add_qtc_plugin(CppTools
  DEPENDS Qt5::Network Qt5::Xml
  PUBLIC_DEPENDS CPlusPlus Qt5::Widgets
  PLUGIN_DEPENDS Core ProjectExplorer TextEditor
  SOURCES
    abstracteditorsupport.cpp abstracteditorsupport.h
    abstractoverviewmodel.h
    baseeditordocumentparser.cpp baseeditordocumentparser.h
    baseeditordocumentprocessor.cpp baseeditordocumentprocessor.h
    builtincursorinfo.cpp builtincursorinfo.h
    builtineditordocumentparser.cpp builtineditordocumentparser.h
    builtineditordocumentprocessor.cpp builtineditordocumentprocessor.h
    builtinindexingsupport.cpp builtinindexingsupport.h
    clangbasechecks.ui
    clangdiagnosticconfig.cpp clangdiagnosticconfig.h
    clangdiagnosticconfigsmodel.cpp clangdiagnosticconfigsmodel.h
    clangdiagnosticconfigsselectionwidget.cpp clangdiagnosticconfigsselectionwidget.h
    clangdiagnosticconfigswidget.cpp clangdiagnosticconfigswidget.h clangdiagnosticconfigswidget.ui
    compileroptionsbuilder.cpp compileroptionsbuilder.h
    cppbuiltinmodelmanagersupport.cpp cppbuiltinmodelmanagersupport.h
    cppcanonicalsymbol.cpp cppcanonicalsymbol.h
    cppchecksymbols.cpp cppchecksymbols.h
    cppclassesfilter.cpp cppclassesfilter.h
    cppcodeformatter.cpp cppcodeformatter.h
    cppcodemodelinspectordumper.cpp cppcodemodelinspectordumper.h
    cppcodemodelsettings.cpp cppcodemodelsettings.h
    cppcodemodelsettingspage.cpp cppcodemodelsettingspage.h cppcodemodelsettingspage.ui
    cppcodestylepreferences.cpp cppcodestylepreferences.h
    cppcodestylepreferencesfactory.cpp cppcodestylepreferencesfactory.h
    cppcodestylesettings.cpp cppcodestylesettings.h
    cppcodestylesettingspage.cpp cppcodestylesettingspage.h cppcodestylesettingspage.ui
    cppcodestylesnippets.h
    cppcompletionassist.cpp cppcompletionassist.h
    cppcompletionassistprocessor.cpp cppcompletionassistprocessor.h
    cppcompletionassistprovider.cpp cppcompletionassistprovider.h
    cppcurrentdocumentfilter.cpp cppcurrentdocumentfilter.h
    cppcursorinfo.h
    cppdoxygen.cpp cppdoxygen.h
    cppeditoroutline.cpp cppeditoroutline.h
    cppeditorwidgetinterface.h
    cppelementevaluator.cpp cppelementevaluator.h
    cppfileiterationorder.cpp cppfileiterationorder.h
    cppfilesettingspage.cpp cppfilesettingspage.h cppfilesettingspage.ui
    cppfindreferences.cpp cppfindreferences.h
    cppfollowsymbolundercursor.cpp cppfollowsymbolundercursor.h
    cppfunctionsfilter.cpp cppfunctionsfilter.h
    cpphoverhandler.cpp cpphoverhandler.h
    cppincludesfilter.cpp cppincludesfilter.h
    cppindexingsupport.cpp cppindexingsupport.h
    cpplocalsymbols.cpp cpplocalsymbols.h
    cpplocatordata.cpp cpplocatordata.h
    cpplocatorfilter.cpp cpplocatorfilter.h
    cppmodelmanager.cpp cppmodelmanager.h
    cppmodelmanagerinterface.h
    cppmodelmanagersupport.cpp cppmodelmanagersupport.h
    cppoverviewmodel.cpp cppoverviewmodel.h
    cpppointerdeclarationformatter.cpp cpppointerdeclarationformatter.h
    cppprojectfile.cpp cppprojectfile.h
    cppprojectfilecategorizer.cpp cppprojectfilecategorizer.h
    cppprojectinfogenerator.cpp cppprojectinfogenerator.h
    cppprojectpartchooser.cpp cppprojectpartchooser.h
    cppprojectupdater.cpp cppprojectupdater.h
    cppprojectupdaterinterface.h
    cppqtstyleindenter.cpp cppqtstyleindenter.h
    cpprefactoringchanges.cpp cpprefactoringchanges.h
    cpprefactoringengine.cpp cpprefactoringengine.h
    cppselectionchanger.cpp cppselectionchanger.h
    cppsemanticinfo.h
    cppsemanticinfoupdater.cpp cppsemanticinfoupdater.h
    cppsourceprocessor.cpp cppsourceprocessor.h
    cppsymbolinfo.h
    cpptools_global.h
    cpptools_utils.h
    cpptoolsbridge.cpp cpptoolsbridge.h
    cpptoolsbridgeinterface.h
    cpptoolsbridgeqtcreatorimplementation.cpp cpptoolsbridgeqtcreatorimplementation.h
    cpptoolsconstants.h
    cpptoolsjsextension.cpp cpptoolsjsextension.h
    cpptoolsplugin.cpp cpptoolsplugin.h
    cpptoolsreuse.cpp cpptoolsreuse.h
    cpptoolssettings.cpp cpptoolssettings.h
    cppvirtualfunctionassistprovider.cpp cppvirtualfunctionassistprovider.h
    cppvirtualfunctionproposalitem.cpp cppvirtualfunctionproposalitem.h
    cppworkingcopy.cpp cppworkingcopy.h
    cursorineditor.h
    doxygengenerator.cpp doxygengenerator.h
    editordocumenthandle.cpp editordocumenthandle.h
    followsymbolinterface.h
    functionutils.cpp functionutils.h
    generatedcodemodelsupport.cpp generatedcodemodelsupport.h
    headerpathfilter.cpp headerpathfilter.h
    includeutils.cpp includeutils.h
    indexitem.cpp indexitem.h
    insertionpointlocator.cpp insertionpointlocator.h
    projectinfo.cpp projectinfo.h
    projectpart.cpp projectpart.h
    refactoringengineinterface.h
    searchsymbols.cpp searchsymbols.h
    semantichighlighter.cpp semantichighlighter.h
    senddocumenttracker.cpp senddocumenttracker.h
    stringtable.cpp stringtable.h
    symbolfinder.cpp symbolfinder.h
    symbolsfindfilter.cpp symbolsfindfilter.h
    typehierarchybuilder.cpp typehierarchybuilder.h
    usages.h
    wrappablelineedit.cpp wrappablelineedit.h
)

extend_qtc_plugin(CppTools
  CONDITION WITH_TESTS
  SOURCES
    cppcodegen_test.cpp
    cppcompletion_test.cpp
    cppheadersource_test.cpp
    cpplocalsymbols_test.cpp
    cpplocatorfilter_test.cpp
    cppmodelmanager_test.cpp
    cpppointerdeclarationformatter_test.cpp
    cppsourceprocessertesthelper.cpp cppsourceprocessertesthelper.h
    cppsourceprocessor_test.cpp
    cpptoolstestcase.cpp cpptoolstestcase.h
    modelmanagertesthelper.cpp modelmanagertesthelper.h
    symbolsearcher_test.cpp
    typehierarchybuilder_test.cpp
)
