add_qtc_library(3rd_cplusplus OBJECT
  PUBLIC_DEPENDS Qt::Core Utils
  SOURCES
    AST.cpp AST.h
    ASTClone.cpp
    ASTMatch0.cpp
    ASTMatcher.cpp ASTMatcher.h
    ASTPatternBuilder.h
    ASTVisit.cpp
    ASTVisitor.cpp ASTVisitor.h
    ASTfwd.h
    Bind.cpp Bind.h
    CPlusPlus.h
    CPlusPlusForwardDeclarations.h
    Control.cpp Control.h
    CoreTypes.cpp CoreTypes.h
    DiagnosticClient.cpp DiagnosticClient.h
    FullySpecifiedType.cpp FullySpecifiedType.h
    Keywords.cpp
    Lexer.cpp Lexer.h
    LiteralTable.h
    Literals.cpp Literals.h
    Matcher.cpp Matcher.h
    MemoryPool.cpp MemoryPool.h
    Name.cpp Name.h
    NameVisitor.cpp NameVisitor.h
    Names.cpp Names.h
    ObjectiveCAtKeywords.cpp
    ObjectiveCTypeQualifiers.cpp ObjectiveCTypeQualifiers.h
    Parser.cpp Parser.h
    QtContextKeywords.cpp QtContextKeywords.h
    SafeMatcher.cpp SafeMatcher.h
    Scope.cpp Scope.h
    Symbol.cpp Symbol.h
    SymbolVisitor.h
    Symbols.cpp Symbols.h
    Templates.cpp Templates.h
    Token.cpp Token.h
    TranslationUnit.cpp TranslationUnit.h
    Type.cpp Type.h
    TypeVisitor.cpp TypeVisitor.h
    cppassert.h
  SKIP_PCH
)

set(export_symbol_declaration DEFINES CPLUSPLUS_BUILD_LIB)
if (QTC_STATIC_BUILD)
  set(export_symbol_declaration PUBLIC_DEFINES CPLUSPLUS_BUILD_STATIC_LIB)
endif()
extend_qtc_target(3rd_cplusplus ${export_symbol_declaration})

if(TARGET 3rd_cplusplus)
  qtc_enable_release_for_debug_configuration()
  if (BUILD_WITH_PCH)
    target_precompile_headers(3rd_cplusplus PRIVATE
      "${QtCreator_SOURCE_DIR}/src/shared/qtcreator_gui_pch.h")
  endif()

  if(UNIX)
    target_compile_options(3rd_cplusplus PRIVATE "-Wno-unused-variable")
  endif()
endif()
