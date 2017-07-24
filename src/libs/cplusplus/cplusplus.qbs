import qbs 1.0

Project {
    name: "CPlusPlus"

    QtcDevHeaders { }
    QtcDevHeaders {
        productName: "cplusplus (3rd party)"
        baseDir: sourceDirectory + "/../3rdparty/cplusplus"
    }

    QtcLibrary {
        cpp.includePaths: base.concat("../3rdparty")
        cpp.defines: base.concat([
            "NDEBUG",
            "CPLUSPLUS_BUILD_LIB"
        ])
        cpp.optimization: "fast"

        Depends { name: "Qt.widgets" }
        Depends { name: "Utils" }

        Group {
            name: "ThirdPartyCPlusPlus"
            prefix: "../3rdparty/cplusplus/"
            files: [
                "AST.cpp",
                "AST.h",
                "ASTClone.cpp",
                "ASTMatch0.cpp",
                "ASTMatcher.cpp",
                "ASTMatcher.h",
                "ASTPatternBuilder.h",
                "ASTVisit.cpp",
                "ASTVisitor.cpp",
                "ASTVisitor.h",
                "ASTfwd.h",
                "Bind.cpp",
                "Bind.h",
                "CPlusPlus.h",
                "Control.cpp",
                "Control.h",
                "CoreTypes.cpp",
                "CoreTypes.h",
                "DiagnosticClient.cpp",
                "DiagnosticClient.h",
                "FullySpecifiedType.cpp",
                "FullySpecifiedType.h",
                "Keywords.cpp",
                "Lexer.cpp",
                "Lexer.h",
                "LiteralTable.h",
                "Literals.cpp",
                "Literals.h",
                "Matcher.cpp",
                "Matcher.h",
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
                "ObjectiveCTypeQualifiers.h",
                "Parser.cpp",
                "Parser.h",
                "QtContextKeywords.cpp",
                "QtContextKeywords.h",
                "SafeMatcher.cpp",
                "SafeMatcher.h",
                "Scope.cpp",
                "Scope.h",
                "Symbol.cpp",
                "Symbol.h",
                "SymbolVisitor.h",
                "Symbols.cpp",
                "Symbols.h",
                "Templates.cpp",
                "Templates.h",
                "Token.cpp",
                "Token.h",
                "TranslationUnit.cpp",
                "TranslationUnit.h",
                "Type.cpp",
                "Type.h",
                "TypeVisitor.cpp",
                "TypeVisitor.h",
            ]
        }

        Group {
            name: "General"
            files: [
                "AlreadyConsideredClassContainer.h",
                "ASTParent.cpp", "ASTParent.h",
                "ASTPath.cpp", "ASTPath.h",
                "BackwardsScanner.cpp", "BackwardsScanner.h",
                "CppDocument.cpp", "CppDocument.h",
                "CppRewriter.cpp", "CppRewriter.h",
                "cppmodelmanagerbase.cpp", "cppmodelmanagerbase.h",
                "DependencyTable.cpp", "DependencyTable.h",
                "DeprecatedGenTemplateInstance.cpp", "DeprecatedGenTemplateInstance.h",
                "ExpressionUnderCursor.cpp", "ExpressionUnderCursor.h",
                "FastPreprocessor.cpp", "FastPreprocessor.h",
                "FindUsages.cpp", "FindUsages.h",
                "Icons.cpp", "Icons.h",
                "LookupContext.cpp", "LookupContext.h",
                "LookupItem.cpp", "LookupItem.h",
                "Macro.cpp", "Macro.h",
                "MatchingText.cpp", "MatchingText.h",
                "NamePrettyPrinter.cpp", "NamePrettyPrinter.h",
                "Overview.cpp", "Overview.h",
                "OverviewModel.cpp", "OverviewModel.h",
                "PPToken.cpp", "PPToken.h",
                "PreprocessorClient.cpp", "PreprocessorClient.h",
                "PreprocessorEnvironment.cpp", "PreprocessorEnvironment.h",
                "ResolveExpression.cpp", "ResolveExpression.h",
                "SimpleLexer.cpp", "SimpleLexer.h",
                "SnapshotSymbolVisitor.cpp", "SnapshotSymbolVisitor.h",
                "SymbolNameVisitor.cpp", "SymbolNameVisitor.h",
                "TypeOfExpression.cpp", "TypeOfExpression.h",
                "TypePrettyPrinter.cpp", "TypePrettyPrinter.h",
                "cplusplus.qrc",
                "findcdbbreakpoint.cpp", "findcdbbreakpoint.h",
                "pp-cctype.h",
                "pp-engine.cpp", "pp-engine.h",
                "pp-scanner.cpp", "pp-scanner.h",
                "pp.h",
            ]
        }

        Export {
            cpp.includePaths: [
                product.sourceDirectory + "/../3rdparty"
            ]
        }
    }
}
