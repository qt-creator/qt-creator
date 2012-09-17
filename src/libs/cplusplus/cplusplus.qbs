import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "CPlusPlus"

    cpp.includePaths: [
        ".",
        "..",
        "../3rdparty/cplusplus",
        "../../plugins"
    ]
    cpp.defines: base.concat([
        "NDEBUG",
        "CPLUSPLUS_BUILD_LIB"
    ])
    cpp.optimization: "fast"

    Depends { name: "cpp" }
    Depends { name: "Qt.widgets" }

    Group {
        prefix: "../3rdparty/cplusplus/"
        files: [
            "ASTPatternBuilder.cpp",
            "CPlusPlus.h",
            "LiteralTable.cpp",
            "ObjectiveCTypeQualifiers.h",
            "Templates.cpp",
            "Templates.h",
            "ASTVisitor.cpp",
            "Control.cpp",
            "Control.h",
            "CoreTypes.cpp",
            "CoreTypes.h",
            "DiagnosticClient.cpp",
            "DiagnosticClient.h",
            "FullySpecifiedType.cpp",
            "FullySpecifiedType.h",
            "Lexer.h",
            "LiteralTable.h",
            "Literals.h",
            "MemoryPool.cpp",
            "MemoryPool.h",
            "Name.cpp",
            "Name.h",
            "NameVisitor.cpp",
            "NameVisitor.h",
            "Names.cpp",
            "Names.h",
            "ObjectiveCAtKeywords.cpp",
            "ObjectiveCTypeQualifiers.cpp",
            "Scope.cpp",
            "Scope.h",
            "SymbolVisitor.cpp",
            "SymbolVisitor.h",
            "Symbols.h",
            "Type.cpp",
            "Type.h",
            "TypeMatcher.cpp",
            "TypeMatcher.h",
            "TypeVisitor.cpp",
            "TypeVisitor.h",
            "AST.cpp",
            "AST.h",
            "ASTClone.cpp",
            "ASTMatch0.cpp",
            "ASTMatcher.cpp",
            "ASTMatcher.h",
            "ASTPatternBuilder.h",
            "ASTVisit.cpp",
            "ASTVisitor.h",
            "ASTfwd.h",
            "Bind.cpp",
            "Bind.h",
            "Keywords.cpp",
            "Lexer.cpp",
            "Literals.cpp",
            "Parser.cpp",
            "Parser.h",
            "QtContextKeywords.cpp",
            "QtContextKeywords.h",
            "Symbol.cpp",
            "Symbol.h",
            "Symbols.cpp",
            "Token.cpp",
            "Token.h",
            "TranslationUnit.cpp",
            "TranslationUnit.h"
        ]
    }

    files: [
        "cplusplus.qrc",
        "ASTParent.cpp",
        "ASTParent.h",
        "ASTPath.cpp",
        "ASTPath.h",
        "BackwardsScanner.cpp",
        "BackwardsScanner.h",
        "CppDocument.cpp",
        "CppDocument.h",
        "CppRewriter.cpp",
        "CppRewriter.h",
        "DependencyTable.cpp",
        "DependencyTable.h",
        "DeprecatedGenTemplateInstance.cpp",
        "DeprecatedGenTemplateInstance.h",
        "ExpressionUnderCursor.cpp",
        "ExpressionUnderCursor.h",
        "FastPreprocessor.cpp",
        "FastPreprocessor.h",
        "FindUsages.cpp",
        "FindUsages.h",
        "Icons.cpp",
        "Icons.h",
        "LookupContext.cpp",
        "LookupContext.h",
        "LookupItem.cpp",
        "LookupItem.h",
        "Macro.cpp",
        "Macro.h",
        "MatchingText.cpp",
        "MatchingText.h",
        "NamePrettyPrinter.cpp",
        "NamePrettyPrinter.h",
        "Overview.cpp",
        "Overview.h",
        "OverviewModel.cpp",
        "OverviewModel.h",
        "PreprocessorClient.cpp",
        "PreprocessorClient.h",
        "PreprocessorEnvironment.cpp",
        "PreprocessorEnvironment.h",
        "ResolveExpression.cpp",
        "ResolveExpression.h",
        "SimpleLexer.cpp",
        "SimpleLexer.h",
        "SnapshotSymbolVisitor.cpp",
        "SnapshotSymbolVisitor.h",
        "SymbolNameVisitor.cpp",
        "SymbolNameVisitor.h",
        "TypeOfExpression.cpp",
        "TypeOfExpression.h",
        "TypePrettyPrinter.cpp",
        "TypePrettyPrinter.h",
        "findcdbbreakpoint.cpp",
        "findcdbbreakpoint.h",
        "pp-cctype.h",
        "pp-engine.cpp",
        "pp-engine.h",
        "PPToken.cpp",
        "PPToken.h",
        "pp-scanner.cpp",
        "pp-scanner.h",
        "pp.h",
        "images/class.png",
        "images/enum.png",
        "images/enumerator.png",
        "images/func.png",
        "images/func_priv.png",
        "images/func_prot.png",
        "images/keyword.png",
        "images/macro.png",
        "images/namespace.png",
        "images/signal.png",
        "images/slot.png",
        "images/slot_priv.png",
        "images/slot_prot.png",
        "images/var.png",
        "images/var_priv.png",
        "images/var_prot.png"
    ]

    ProductModule {
        Depends { name: "cpp" }
        cpp.includePaths: [
            ".",
            "../3rdparty",
            "../3rdparty/cplusplus"
        ]
    }
}

