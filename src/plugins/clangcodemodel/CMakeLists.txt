set(TEST_LINK_DEPENDS)
if(WITH_TESTS)
  set(TEST_LINK_DEPENDS QtSupport)
endif()

add_qtc_plugin(ClangCodeModel
  CONDITION TARGET libclang
  DEPENDS ClangSupport CPlusPlus
  PLUGIN_DEPENDS Core CppEditor LanguageClient ${TEST_LINK_DEPENDS} TextEditor
  PLUGIN_TEST_DEPENDS QmakeProjectManager
  SOURCES
    clangactivationsequencecontextprocessor.cpp clangactivationsequencecontextprocessor.h
    clangactivationsequenceprocessor.cpp clangactivationsequenceprocessor.h
    clangassistproposalitem.cpp clangassistproposalitem.h
    clangassistproposalmodel.cpp clangassistproposalmodel.h
    clangbackendcommunicator.cpp clangbackendcommunicator.h
    clangbackendlogging.cpp clangbackendlogging.h
    clangbackendreceiver.cpp clangbackendreceiver.h
    clangbackendsender.cpp clangbackendsender.h
    clangcodemodelplugin.cpp clangcodemodelplugin.h
    clangcompletionassistinterface.cpp clangcompletionassistinterface.h
    clangcompletionassistprocessor.cpp clangcompletionassistprocessor.h
    clangcompletionassistprovider.cpp clangcompletionassistprovider.h
    clangcompletionchunkstotextconverter.cpp clangcompletionchunkstotextconverter.h
    clangcompletioncontextanalyzer.cpp clangcompletioncontextanalyzer.h
    clangconstants.h
    clangcurrentdocumentfilter.cpp clangcurrentdocumentfilter.h
    clangdclient.cpp clangdclient.h
    clangdiagnosticfilter.cpp clangdiagnosticfilter.h
    clangdiagnosticmanager.cpp clangdiagnosticmanager.h
    clangdiagnostictooltipwidget.cpp clangdiagnostictooltipwidget.h
    clangdquickfixfactory.cpp clangdquickfixfactory.h
    clangdqpropertyhighlighter.cpp clangdqpropertyhighlighter.h
    clangeditordocumentparser.cpp clangeditordocumentparser.h
    clangeditordocumentprocessor.cpp clangeditordocumentprocessor.h
    clangfixitoperation.cpp clangfixitoperation.h
    clangfixitoperationsextractor.cpp clangfixitoperationsextractor.h
    clangfollowsymbol.cpp clangfollowsymbol.h
    clangfunctionhintmodel.cpp clangfunctionhintmodel.h
    clangdlocatorfilters.cpp clangdlocatorfilters.h
    clanghighlightingresultreporter.cpp clanghighlightingresultreporter.h
    clanghoverhandler.cpp clanghoverhandler.h
    clangisdiagnosticrelatedtolocation.h
    clangmodelmanagersupport.cpp clangmodelmanagersupport.h
    clangoverviewmodel.cpp clangoverviewmodel.h
    clangpreprocessorassistproposalitem.cpp clangpreprocessorassistproposalitem.h
    clangprojectsettings.cpp clangprojectsettings.h
    clangprojectsettingswidget.cpp clangprojectsettingswidget.h clangprojectsettingswidget.ui
    clangrefactoringengine.cpp clangrefactoringengine.h
    clangtextmark.cpp clangtextmark.h
    clanguiheaderondiskmanager.cpp clanguiheaderondiskmanager.h
    clangutils.cpp clangutils.h
  EXPLICIT_MOC clangcodemodelplugin.h
)

extend_qtc_plugin(ClangCodeModel
  SOURCES
    moc/parser.cpp moc/parser.h
    moc/preprocessor.cpp moc/preprocessor.h
    moc/symbols.h
    moc/token.cpp moc/token.h
    moc/utils.h
)

extend_qtc_plugin(ClangCodeModel
  CONDITION WITH_TESTS
  SOURCES
    test/clangautomationutils.cpp test/clangautomationutils.h
    test/clangbatchfileprocessor.cpp test/clangbatchfileprocessor.h
    test/clangcodecompletion_test.cpp test/clangcodecompletion_test.h
    test/clangdtests.cpp test/clangdtests.h
    test/data/clangtestdata.qrc
)

if(MINGW)
  set(big_obj_compile_option "-Wa,-mbig-obj")
elseif(MSVC)
  set(big_obj_compile_option "/bigobj")
endif()

extend_qtc_plugin(ClangCodeModel
  CONDITION DEFINED big_obj_compile_option
  PROPERTIES COMPILE_OPTIONS ${big_obj_compile_option}
)
