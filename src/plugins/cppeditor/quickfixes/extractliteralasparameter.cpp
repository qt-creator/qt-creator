// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extractliteralasparameter.h"

#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"
#include "cppquickfixhelpers.h"

#include <cplusplus/ASTPath.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#include <QtTest>
#endif

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

struct ReplaceLiteralsResult
{
    Token token;
    QString literalText;
};

template <class T>
class ReplaceLiterals : private ASTVisitor
{
public:
    ReplaceLiterals(const CppRefactoringFilePtr &file, ChangeSet *changes, T *literal)
        : ASTVisitor(file->cppDocument()->translationUnit()), m_file(file), m_changes(changes),
        m_literal(literal)
    {
        m_result.token = m_file->tokenAt(literal->firstToken());
        m_literalTokenText = m_result.token.spell();
        m_result.literalText = QLatin1String(m_literalTokenText);
        if (m_result.token.isCharLiteral()) {
            m_result.literalText.prepend(QLatin1Char('\''));
            m_result.literalText.append(QLatin1Char('\''));
            if (m_result.token.kind() == T_WIDE_CHAR_LITERAL)
                m_result.literalText.prepend(QLatin1Char('L'));
            else if (m_result.token.kind() == T_UTF16_CHAR_LITERAL)
                m_result.literalText.prepend(QLatin1Char('u'));
            else if (m_result.token.kind() == T_UTF32_CHAR_LITERAL)
                m_result.literalText.prepend(QLatin1Char('U'));
        } else if (m_result.token.isStringLiteral()) {
            m_result.literalText.prepend(QLatin1Char('"'));
            m_result.literalText.append(QLatin1Char('"'));
            if (m_result.token.kind() == T_WIDE_STRING_LITERAL)
                m_result.literalText.prepend(QLatin1Char('L'));
            else if (m_result.token.kind() == T_UTF16_STRING_LITERAL)
                m_result.literalText.prepend(QLatin1Char('u'));
            else if (m_result.token.kind() == T_UTF32_STRING_LITERAL)
                m_result.literalText.prepend(QLatin1Char('U'));
        }
    }

    ReplaceLiteralsResult apply(AST *ast)
    {
        ast->accept(this);
        return m_result;
    }

private:
    bool visit(T *ast) override
    {
        if (ast != m_literal
            && strcmp(m_file->tokenAt(ast->firstToken()).spell(), m_literalTokenText) != 0) {
            return true;
        }
        int start, end;
        m_file->startAndEndOf(ast->firstToken(), &start, &end);
        m_changes->replace(start, end, QLatin1String("newParameter"));
        return true;
    }

    const CppRefactoringFilePtr &m_file;
    ChangeSet *m_changes;
    T *m_literal;
    const char *m_literalTokenText;
    ReplaceLiteralsResult m_result;
};

class ExtractLiteralAsParameterOp : public CppQuickFixOperation
{
public:
    ExtractLiteralAsParameterOp(const CppQuickFixInterface &interface, int priority,
                                ExpressionAST *literal, FunctionDefinitionAST *function)
        : CppQuickFixOperation(interface, priority),
        m_literal(literal),
        m_functionDefinition(function)
    {
        setDescription(Tr::tr("Extract Constant as Function Parameter"));
    }

    struct FoundDeclaration
    {
        FunctionDeclaratorAST *ast = nullptr;
        CppRefactoringFilePtr file;
    };

    FoundDeclaration findDeclaration(const CppRefactoringChanges &refactoring,
                                     FunctionDefinitionAST *ast)
    {
        FoundDeclaration result;
        Function *func = ast->symbol;
        if (Class *matchingClass = isMemberFunction(context(), func)) {
            // Dealing with member functions
            const QualifiedNameId *qName = func->name()->asQualifiedNameId();
            for (Symbol *s = matchingClass->find(qName->identifier()); s; s = s->next()) {
                if (!s->name()
                    || !qName->identifier()->match(s->identifier())
                    || !s->type()->asFunctionType()
                    || !s->type().match(func->type())
                    || s->asFunction()) {
                    continue;
                }

                const FilePath declFilePath = matchingClass->filePath();
                result.file = refactoring.cppFile(declFilePath);
                ASTPath astPath(result.file->cppDocument());
                const QList<AST *> path = astPath(s->line(), s->column());
                SimpleDeclarationAST *simpleDecl = nullptr;
                for (AST *node : path) {
                    simpleDecl = node->asSimpleDeclaration();
                    if (simpleDecl) {
                        if (simpleDecl->symbols && !simpleDecl->symbols->next) {
                            result.ast = functionDeclarator(simpleDecl);
                            return result;
                        }
                    }
                }

                if (simpleDecl)
                    break;
            }
        } else if (Namespace *matchingNamespace = isNamespaceFunction(context(), func)) {
            // Dealing with free functions and inline member functions.
            bool isHeaderFile;
            FilePath declFilePath = correspondingHeaderOrSource(filePath(), &isHeaderFile);
            if (!declFilePath.exists())
                return FoundDeclaration();
            result.file = refactoring.cppFile(declFilePath);
            if (!result.file)
                return FoundDeclaration();
            const LookupContext lc(result.file->cppDocument(), snapshot());
            const QList<LookupItem> candidates = lc.lookup(func->name(), matchingNamespace);
            for (const LookupItem &candidate : candidates) {
                if (Symbol *s = candidate.declaration()) {
                    if (s->asDeclaration()) {
                        ASTPath astPath(result.file->cppDocument());
                        const QList<AST *> path = astPath(s->line(), s->column());
                        for (AST *node : path) {
                            SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration();
                            if (simpleDecl) {
                                result.ast = functionDeclarator(simpleDecl);
                                return result;
                            }
                        }
                    }
                }
            }
        }
        return result;
    }

    void perform() override
    {
        FunctionDeclaratorAST *functionDeclaratorOfDefinition
            = functionDeclarator(m_functionDefinition);
        const CppRefactoringChanges refactoring(snapshot());
        deduceTypeNameOfLiteral(currentFile()->cppDocument());

        ChangeSet changes;
        if (NumericLiteralAST *concreteLiteral = m_literal->asNumericLiteral()) {
            m_literalInfo = ReplaceLiterals<NumericLiteralAST>(currentFile(), &changes,
                                                               concreteLiteral)
                                .apply(m_functionDefinition->function_body);
        } else if (StringLiteralAST *concreteLiteral = m_literal->asStringLiteral()) {
            m_literalInfo = ReplaceLiterals<StringLiteralAST>(currentFile(), &changes,
                                                              concreteLiteral)
                                .apply(m_functionDefinition->function_body);
        } else if (BoolLiteralAST *concreteLiteral = m_literal->asBoolLiteral()) {
            m_literalInfo = ReplaceLiterals<BoolLiteralAST>(currentFile(), &changes,
                                                            concreteLiteral)
                                .apply(m_functionDefinition->function_body);
        }
        const FoundDeclaration functionDeclaration
            = findDeclaration(refactoring, m_functionDefinition);
        appendFunctionParameter(functionDeclaratorOfDefinition, currentFile(), &changes,
                                !functionDeclaration.ast);
        if (functionDeclaration.ast) {
            if (currentFile()->filePath() != functionDeclaration.file->filePath()) {
                ChangeSet declChanges;
                appendFunctionParameter(functionDeclaration.ast, functionDeclaration.file, &declChanges,
                                        true);
                functionDeclaration.file->apply(declChanges);
            } else {
                appendFunctionParameter(functionDeclaration.ast, currentFile(), &changes,
                                        true);
            }
        }
        currentFile()->apply(changes);
        QTextCursor c = currentFile()->cursor();
        c.setPosition(c.position() - parameterName().length());
        editor()->setTextCursor(c);
        editor()->renameSymbolUnderCursor();
    }

private:
    bool hasParameters(FunctionDeclaratorAST *ast) const
    {
        return ast->parameter_declaration_clause
               && ast->parameter_declaration_clause->parameter_declaration_list
               && ast->parameter_declaration_clause->parameter_declaration_list->value;
    }

    void deduceTypeNameOfLiteral(const Document::Ptr &document)
    {
        TypeOfExpression typeOfExpression;
        typeOfExpression.init(document, snapshot());
        Overview overview;
        Scope *scope = m_functionDefinition->symbol->enclosingScope();
        const QList<LookupItem> items = typeOfExpression(m_literal, document, scope);
        if (!items.isEmpty())
            m_typeName = overview.prettyType(items.first().type());
    }

    static QString parameterName() { return QLatin1String("newParameter"); }

    QString parameterDeclarationTextToInsert(FunctionDeclaratorAST *ast) const
    {
        QString str;
        if (hasParameters(ast))
            str = QLatin1String(", ");
        str += m_typeName;
        if (!m_typeName.endsWith(QLatin1Char('*')))
            str += QLatin1Char(' ');
        str += parameterName();
        return str;
    }

    FunctionDeclaratorAST *functionDeclarator(SimpleDeclarationAST *ast) const
    {
        for (DeclaratorListAST *decls = ast->declarator_list; decls; decls = decls->next) {
            FunctionDeclaratorAST * const functionDeclaratorAST = functionDeclarator(decls->value);
            if (functionDeclaratorAST)
                return functionDeclaratorAST;
        }
        return nullptr;
    }

    FunctionDeclaratorAST *functionDeclarator(DeclaratorAST *ast) const
    {
        for (PostfixDeclaratorListAST *pds = ast->postfix_declarator_list; pds; pds = pds->next) {
            FunctionDeclaratorAST *funcdecl = pds->value->asFunctionDeclarator();
            if (funcdecl)
                return funcdecl;
        }
        return nullptr;
    }

    FunctionDeclaratorAST *functionDeclarator(FunctionDefinitionAST *ast) const
    {
        return functionDeclarator(ast->declarator);
    }

    void appendFunctionParameter(FunctionDeclaratorAST *ast, const CppRefactoringFileConstPtr &file,
                                 ChangeSet *changes, bool addDefaultValue)
    {
        if (!ast)
            return;
        if (m_declarationInsertionString.isEmpty())
            m_declarationInsertionString = parameterDeclarationTextToInsert(ast);
        QString insertion = m_declarationInsertionString;
        if (addDefaultValue)
            insertion += QLatin1String(" = ") + m_literalInfo.literalText;
        changes->insert(file->startOf(ast->rparen_token), insertion);
    }

    ExpressionAST *m_literal;
    FunctionDefinitionAST *m_functionDefinition;
    QString m_typeName;
    QString m_declarationInsertionString;
    ReplaceLiteralsResult m_literalInfo;
};

/*!
  Extracts the selected constant and converts it to a parameter of the current function.
  Activates on numeric, bool, character, or string literal in the function body.
 */
class ExtractLiteralAsParameter : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        if (path.count() < 2)
            return;

        AST * const lastAst = path.last();
        ExpressionAST *literal;
        if (!((literal = lastAst->asNumericLiteral())
              || (literal = lastAst->asStringLiteral())
              || (literal = lastAst->asBoolLiteral()))) {
            return;
        }

        FunctionDefinitionAST *function;
        int i = path.count() - 2;
        while (!(function = path.at(i)->asFunctionDefinition())) {
            // Ignore literals in lambda expressions for now.
            if (path.at(i)->asLambdaExpression())
                return;
            if (--i < 0)
                return;
        }

        PostfixDeclaratorListAST * const declaratorList = function->declarator->postfix_declarator_list;
        if (!declaratorList)
            return;
        if (FunctionDeclaratorAST *declarator = declaratorList->value->asFunctionDeclarator()) {
            if (declarator->parameter_declaration_clause
                && declarator->parameter_declaration_clause->dot_dot_dot_token) {
                // Do not handle functions with ellipsis parameter.
                return;
            }
        }

        const int priority = path.size() - 1;
        result << new ExtractLiteralAsParameterOp(interface, priority, literal, function);
    }
};

#ifdef WITH_TESTS
using namespace Tests;

class ExtractLiteralAsParameterTest : public QObject
{
    Q_OBJECT

private slots:
    void testTypeDeduction_data()
    {
        QTest::addColumn<QByteArray>("typeString");
        QTest::addColumn<QByteArray>("literal");
        QTest::newRow("int")
            << QByteArray("int ") << QByteArray("156");
        QTest::newRow("unsigned int")
            << QByteArray("unsigned int ") << QByteArray("156u");
        QTest::newRow("long")
            << QByteArray("long ") << QByteArray("156l");
        QTest::newRow("unsigned long")
            << QByteArray("unsigned long ") << QByteArray("156ul");
        QTest::newRow("long long")
            << QByteArray("long long ") << QByteArray("156ll");
        QTest::newRow("unsigned long long")
            << QByteArray("unsigned long long ") << QByteArray("156ull");
        QTest::newRow("float")
            << QByteArray("float ") << QByteArray("3.14159f");
        QTest::newRow("double")
            << QByteArray("double ") << QByteArray("3.14159");
        QTest::newRow("long double")
            << QByteArray("long double ") << QByteArray("3.14159L");
        QTest::newRow("bool")
            << QByteArray("bool ") << QByteArray("true");
        QTest::newRow("bool")
            << QByteArray("bool ") << QByteArray("false");
        QTest::newRow("char")
            << QByteArray("char ") << QByteArray("'X'");
        QTest::newRow("wchar_t")
            << QByteArray("wchar_t ") << QByteArray("L'X'");
        QTest::newRow("char16_t")
            << QByteArray("char16_t ") << QByteArray("u'X'");
        QTest::newRow("char32_t")
            << QByteArray("char32_t ") << QByteArray("U'X'");
        QTest::newRow("const char *")
            << QByteArray("const char *") << QByteArray("\"narf\"");
        QTest::newRow("const wchar_t *")
            << QByteArray("const wchar_t *") << QByteArray("L\"narf\"");
        QTest::newRow("const char16_t *")
            << QByteArray("const char16_t *") << QByteArray("u\"narf\"");
        QTest::newRow("const char32_t *")
            << QByteArray("const char32_t *") << QByteArray("U\"narf\"");
    }

    void testTypeDeduction()
    {
        QFETCH(QByteArray, typeString);
        QFETCH(QByteArray, literal);
        const QByteArray original = QByteArray("void foo() {return @") + literal + QByteArray(";}\n");
        const QByteArray expected = QByteArray("void foo(") + typeString + QByteArray("newParameter = ")
                                    + literal + QByteArray(") {return newParameter;}\n");

        if (literal == "3.14159") {
            qWarning("Literal 3.14159 is wrongly reported as int. Skipping.");
            return;
        } else if (literal == "3.14159L") {
            qWarning("Literal 3.14159L is wrongly reported as long. Skipping.");
            return;
        }

        ExtractLiteralAsParameter factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    void testFreeFunctionSeparateFiles()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "void foo(const char *a, long b = 1);\n";
        expected =
            "void foo(const char *a, long b = 1, int newParameter = 156);\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "void foo(const char *a, long b)\n"
            "{return 1@56 + 123 + 156;}\n";
        expected =
            "void foo(const char *a, long b, int newParameter)\n"
            "{return newParameter + 123 + newParameter;}\n";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        ExtractLiteralAsParameter factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testMemberFunctionSeparateFiles()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "class Narf {\n"
            "public:\n"
            "    int zort();\n"
            "};\n";
        expected =
            "class Narf {\n"
            "public:\n"
            "    int zort(int newParameter = 155);\n"
            "};\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n\n"
            "int Narf::zort()\n"
            "{ return 15@5 + 1; }\n";
        expected =
            "#include \"file.h\"\n\n"
            "int Narf::zort(int newParameter)\n"
            "{ return newParameter + 1; }\n";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        ExtractLiteralAsParameter factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testNotTriggeringForInvalidCode()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        original =
            "T(\"test\")\n"
            "{\n"
            "    const int i = @14;\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.cpp", original, "");

        ExtractLiteralAsParameter factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void test_data()
    {
        QTest::addColumn<QByteArray>("original");
        QTest::addColumn<QByteArray>("expected");

        QTest::newRow("ExtractLiteralAsParameter_freeFunction")
            << QByteArray(
                   "void foo(const char *a, long b = 1)\n"
                   "{return 1@56 + 123 + 156;}\n")
            << QByteArray(
                   "void foo(const char *a, long b = 1, int newParameter = 156)\n"
                   "{return newParameter + 123 + newParameter;}\n");
        QTest::newRow("ExtractLiteralAsParameter_memberFunction")
            << QByteArray(
                   "class Narf {\n"
                   "public:\n"
                   "    int zort();\n"
                   "};\n\n"
                   "int Narf::zort()\n"
                   "{ return 15@5 + 1; }\n")
            << QByteArray(
                   "class Narf {\n"
                   "public:\n"
                   "    int zort(int newParameter = 155);\n"
                   "};\n\n"
                   "int Narf::zort(int newParameter)\n"
                   "{ return newParameter + 1; }\n");
        QTest::newRow("ExtractLiteralAsParameter_memberFunctionInline")
            << QByteArray(
                   "class Narf {\n"
                   "public:\n"
                   "    int zort()\n"
                   "    { return 15@5 + 1; }\n"
                   "};\n")
            << QByteArray(
                   "class Narf {\n"
                   "public:\n"
                   "    int zort(int newParameter = 155)\n"
                   "    { return newParameter + 1; }\n"
                   "};\n");
    }

    void test()
    {
        QFETCH(QByteArray, original);
        QFETCH(QByteArray, expected);
        ExtractLiteralAsParameter factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

};

QObject *ExtractLiteralAsParameter::createTest() { return new ExtractLiteralAsParameterTest; }

#endif // WITH_TESTS
} // namespace

void registerExtractLiteralAsParameterQuickfix()
{
    CppQuickFixFactory::registerFactory<ExtractLiteralAsParameter>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <extractliteralasparameter.moc>
#endif
