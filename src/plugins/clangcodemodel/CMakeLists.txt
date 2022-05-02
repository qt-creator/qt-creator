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
    clangcodemodelplugin.cpp clangcodemodelplugin.h
    clangcompletioncontextanalyzer.cpp clangcompletioncontextanalyzer.h
    clangconstants.h
    clangdclient.cpp clangdclient.h
    clangdiagnostictooltipwidget.cpp clangdiagnostictooltipwidget.h
    clangdquickfixfactory.cpp clangdquickfixfactory.h
    clangdqpropertyhighlighter.cpp clangdqpropertyhighlighter.h
    clangeditordocumentprocessor.cpp clangeditordocumentprocessor.h
    clangfixitoperation.cpp clangfixitoperation.h
    clangfixitoperationsextractor.cpp clangfixitoperationsextractor.h
    clangdlocatorfilters.cpp clangdlocatorfilters.h
    clangmodelmanagersupport.cpp clangmodelmanagersupport.h
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
    test/clangbatchfileprocessor.cpp test/clangbatchfileprocessor.h
    test/clangdtests.cpp test/clangdtests.h
    test/clangfixittest.cpp test/clangfixittest.h
    test/data/clangtestdata.qrc
)

if(MINGW)
  set(big_obj_compile_option "-Wa,-mbig-obj")
elseif(MSVC)
  set(big_obj_compile_option "/bigobj")
endif()

if(big_obj_compile_option AND TARGET ClangCodeModel)
  set_property(TARGET ClangCodeModel
               APPEND PROPERTY COMPILE_OPTIONS ${big_obj_compile_option})
endif()
