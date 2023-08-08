# TODO: Make Qt::Gui optional -- if really needed that is.

add_qtc_library(CPlusPlus
  DEPENDS Utils Qt::Concurrent
  PUBLIC_DEPENDS 3rd_cplusplus Qt::Gui
  SOURCES
    ASTParent.cpp ASTParent.h
    ASTPath.cpp ASTPath.h
    AlreadyConsideredClassContainer.h
    BackwardsScanner.cpp BackwardsScanner.h
    CppDocument.cpp CppDocument.h
    CppRewriter.cpp CppRewriter.h
    DependencyTable.cpp DependencyTable.h
    DeprecatedGenTemplateInstance.cpp DeprecatedGenTemplateInstance.h
    ExpressionUnderCursor.cpp ExpressionUnderCursor.h
    FastPreprocessor.cpp FastPreprocessor.h
    FindUsages.cpp FindUsages.h
    Icons.cpp Icons.h
    LookupContext.cpp LookupContext.h
    LookupItem.cpp LookupItem.h
    Macro.cpp Macro.h
    MatchingText.cpp MatchingText.h
    NamePrettyPrinter.cpp NamePrettyPrinter.h
    Overview.cpp Overview.h
    PPToken.cpp PPToken.h
    PreprocessorClient.cpp PreprocessorClient.h
    PreprocessorEnvironment.cpp PreprocessorEnvironment.h
    ResolveExpression.cpp ResolveExpression.h
    SimpleLexer.cpp SimpleLexer.h
    SnapshotSymbolVisitor.cpp SnapshotSymbolVisitor.h
    SymbolNameVisitor.cpp SymbolNameVisitor.h
    TypeOfExpression.cpp TypeOfExpression.h
    TypePrettyPrinter.cpp TypePrettyPrinter.h
    cppmodelmanagerbase.cpp cppmodelmanagerbase.h
    declarationcomments.cpp declarationcomments.h
    findcdbbreakpoint.cpp findcdbbreakpoint.h
    pp-cctype.h pp-engine.cpp
    pp-engine.h pp-scanner.cpp
    pp-scanner.h
    pp.h
  SKIP_PCH
)

set(export_symbol_declaration DEFINES CPLUSPLUS_BUILD_LIB)
if (QTC_STATIC_BUILD)
  set(export_symbol_declaration PUBLIC_DEFINES CPLUSPLUS_BUILD_STATIC_LIB)
endif()
extend_qtc_target(CPlusPlus ${export_symbol_declaration})

if(TARGET CPlusPlus)
  qtc_enable_release_for_debug_configuration()
  if (BUILD_WITH_PCH)
    target_precompile_headers(CPlusPlus PRIVATE
      "${QtCreator_SOURCE_DIR}/src/shared/qtcreator_gui_pch.h")
  endif()
endif()
