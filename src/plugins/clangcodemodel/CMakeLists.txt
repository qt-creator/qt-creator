set(TEST_LINK_DEPENDS)
if(WITH_TESTS)
  set(TEST_LINK_DEPENDS QtSupport)
endif()

add_qtc_plugin(ClangCodeModel
  DEPENDS CPlusPlus
  PLUGIN_DEPENDS Core CppEditor LanguageClient ${TEST_LINK_DEPENDS} TextEditor
  PLUGIN_TEST_DEPENDS QmakeProjectManager
  DEFINES
    CLANG_BINDIR="${LLVM_TOOLS_BINARY_DIR}"
  SOURCES
    clangactivationsequencecontextprocessor.cpp clangactivationsequencecontextprocessor.h
    clangactivationsequenceprocessor.cpp clangactivationsequenceprocessor.h
    clangcodemodelplugin.cpp clangcodemodelplugin.h
    clangcodemodeltr.h
    clangcompletioncontextanalyzer.cpp clangcompletioncontextanalyzer.h
    clangconstants.h
    clangdast.cpp clangdast.h
    clangdclient.cpp clangdclient.h
    clangdcompletion.cpp clangdcompletion.h
    clangdfindreferences.cpp clangdfindreferences.h
    clangdfollowsymbol.cpp clangdfollowsymbol.h
    clangdiagnostictooltipwidget.cpp clangdiagnostictooltipwidget.h
    clangdmemoryusagewidget.cpp clangdmemoryusagewidget.h
    clangdquickfixes.cpp clangdquickfixes.h
    clangdqpropertyhighlighter.cpp clangdqpropertyhighlighter.h
    clangdsemantichighlighting.cpp clangdsemantichighlighting.h
    clangdswitchdecldef.cpp clangdswitchdecldef.h
    clangeditordocumentprocessor.cpp clangeditordocumentprocessor.h
    clangfixitoperation.cpp clangfixitoperation.h
    clangdlocatorfilters.cpp clangdlocatorfilters.h
    clangmodelmanagersupport.cpp clangmodelmanagersupport.h
    clangpreprocessorassistproposalitem.cpp clangpreprocessorassistproposalitem.h
    clangtextmark.cpp clangtextmark.h
    clangutils.cpp clangutils.h
    tasktimers.cpp tasktimers.h
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
    test/activationsequenceprocessortest.cpp test/activationsequenceprocessortest.h
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
