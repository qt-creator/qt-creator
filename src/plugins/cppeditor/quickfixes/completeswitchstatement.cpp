// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "completeswitchstatement.h"

#include "../cppeditortr.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"

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

class CaseStatementCollector : public ASTVisitor
{
public:
    CaseStatementCollector(Document::Ptr document, const Snapshot &snapshot,
                           Scope *scope)
        : ASTVisitor(document->translationUnit()),
        document(document),
        scope(scope)
    {
        typeOfExpression.init(document, snapshot);
    }

    QStringList operator ()(AST *ast)
    {
        values.clear();
        foundCaseStatementLevel = false;
        accept(ast);
        return values;
    }

    bool preVisit(AST *ast) override {
        if (CaseStatementAST *cs = ast->asCaseStatement()) {
            foundCaseStatementLevel = true;
            if (ExpressionAST *csExpression = cs->expression) {
                if (ExpressionAST *expression = csExpression->asIdExpression()) {
                    QList<LookupItem> candidates = typeOfExpression(expression, document, scope);
                    if (!candidates.isEmpty() && candidates.first().declaration()) {
                        Symbol *decl = candidates.first().declaration();
                        values << prettyPrint.prettyName(LookupContext::fullyQualifiedName(decl));
                    }
                }
            }
            return true;
        } else if (foundCaseStatementLevel) {
            return false;
        }
        return true;
    }

    Overview prettyPrint;
    bool foundCaseStatementLevel = false;
    QStringList values;
    TypeOfExpression typeOfExpression;
    Document::Ptr document;
    Scope *scope;
};

class CompleteSwitchCaseStatementOp: public CppQuickFixOperation
{
public:
    CompleteSwitchCaseStatementOp(const CppQuickFixInterface &interface,
                                  int priority, CompoundStatementAST *compoundStatement, const QStringList &values)
        : CppQuickFixOperation(interface, priority)
        , compoundStatement(compoundStatement)
        , values(values)
    {
        setDescription(Tr::tr("Complete Switch Statement"));
    }

    void perform() override
    {
        ChangeSet changes;
        int start = currentFile()->endOf(compoundStatement->lbrace_token);
        changes.insert(start, QLatin1String("\ncase ")
                                  + values.join(QLatin1String(":\nbreak;\ncase "))
                                  + QLatin1String(":\nbreak;"));
        currentFile()->apply(changes);
    }

    CompoundStatementAST *compoundStatement;
    QStringList values;
};

static Enum *findEnum(const QList<LookupItem> &results, const LookupContext &ctxt)
{
    for (const LookupItem &result : results) {
        const FullySpecifiedType fst = result.type();

        Type *type = result.declaration() ? result.declaration()->type().type()
                                          : fst.type();

        if (!type)
            continue;
        if (Enum *e = type->asEnumType())
            return e;
        if (const NamedType *namedType = type->asNamedType()) {
            if (ClassOrNamespace *con = ctxt.lookupType(namedType->name(), result.scope())) {
                QList<Enum *> enums = con->unscopedEnums();
                const QList<Symbol *> symbols = con->symbols();
                for (Symbol * const s : symbols) {
                    if (const auto e = s->asEnum())
                        enums << e;
                }
                const Name *referenceName = namedType->name();
                if (const QualifiedNameId *qualifiedName = referenceName->asQualifiedNameId())
                    referenceName = qualifiedName->name();
                for (Enum *e : std::as_const(enums)) {
                    if (const Name *candidateName = e->name()) {
                        if (candidateName->match(referenceName))
                            return e;
                    }
                }
            }
        }
    }

    return nullptr;
}

Enum *conditionEnum(const CppQuickFixInterface &interface, SwitchStatementAST *statement)
{
    Block *block = statement->symbol;
    Scope *scope = interface.semanticInfo().doc->scopeAt(block->line(), block->column());
    TypeOfExpression typeOfExpression;
    typeOfExpression.setExpandTemplates(true);
    typeOfExpression.init(interface.semanticInfo().doc, interface.snapshot());
    const QList<LookupItem> results = typeOfExpression(statement->condition,
                                                       interface.semanticInfo().doc,
                                                       scope);

    return findEnum(results, typeOfExpression.context());
}

//! Adds missing case statements for "switch (enumVariable)"
class CompleteSwitchStatement: public CppQuickFixFactory
{
public:
    CompleteSwitchStatement() { setClangdReplacement({12}); }
#ifdef WITH_TESTS
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();

        if (path.isEmpty())
            return;

        // look for switch statement
        for (int depth = path.size() - 1; depth >= 0; --depth) {
            AST *ast = path.at(depth);
            SwitchStatementAST *switchStatement = ast->asSwitchStatement();
            if (switchStatement) {
                if (!switchStatement->statement || !switchStatement->symbol)
                    return;
                CompoundStatementAST *compoundStatement = switchStatement->statement->asCompoundStatement();
                if (!compoundStatement) // we ignore pathologic case "switch (t) case A: ;"
                    return;
                // look if the condition's type is an enum
                if (Enum *e = conditionEnum(interface, switchStatement)) {
                    // check the possible enum values
                    QStringList values;
                    Overview prettyPrint;
                    for (int i = 0; i < e->memberCount(); ++i) {
                        if (Declaration *decl = e->memberAt(i)->asDeclaration())
                            values << prettyPrint.prettyName(LookupContext::fullyQualifiedName(decl));
                    }
                    // Get the used values
                    Block *block = switchStatement->symbol;
                    CaseStatementCollector caseValues(interface.semanticInfo().doc, interface.snapshot(),
                                                      interface.semanticInfo().doc->scopeAt(block->line(), block->column()));
                    const QStringList usedValues = caseValues(switchStatement);
                    // save the values that would be added
                    for (const QString &usedValue : usedValues)
                        values.removeAll(usedValue);
                    if (!values.isEmpty())
                        result << new CompleteSwitchCaseStatementOp(interface, depth,
                                                                    compoundStatement, values);
                    return;
                }

                return;
            }
        }
    }
};

#ifdef WITH_TESTS
using namespace Tests;

class CompleteSwitchStatementTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArray>("original");
        QTest::addColumn<QByteArray>("expected");

        using QByteArray = QByteArray;

        // Checks: All enum values are added as case statements for a blank switch.
        QTest::newRow("basic1")
            << QByteArray(
                   "enum EnumType { V1, V2 };\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    EnumType t;\n"
                   "    @switch (t) {\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "enum EnumType { V1, V2 };\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    EnumType t;\n"
                   "    switch (t) {\n"
                   "    case V1:\n"
                   "        break;\n"
                   "    case V2:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Same as above for enum class.
        QTest::newRow("basic1_enum class")
            << QByteArray(
                   "enum class EnumType { V1, V2 };\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    EnumType t;\n"
                   "    @switch (t) {\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "enum class EnumType { V1, V2 };\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    EnumType t;\n"
                   "    switch (t) {\n"
                   "    case EnumType::V1:\n"
                   "        break;\n"
                   "    case EnumType::V2:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Same as above with the cursor somewhere in the body.
        QTest::newRow("basic1_enum class, cursor in the body")
            << QByteArray(
                   "enum class EnumType { V1, V2 };\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    EnumType t;\n"
                   "    switch (t) {\n"
                   "    @}\n"
                   "}\n")
            << QByteArray(
                   "enum class EnumType { V1, V2 };\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    EnumType t;\n"
                   "    switch (t) {\n"
                   "    case EnumType::V1:\n"
                   "        break;\n"
                   "    case EnumType::V2:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Checks: All enum values are added as case statements for a blank switch when
        //         the variable is declared alongside the enum definition.
        QTest::newRow("basic1_enum_with_declaration")
            << QByteArray(
                   "enum EnumType { V1, V2 } t;\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    @switch (t) {\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "enum EnumType { V1, V2 } t;\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    switch (t) {\n"
                   "    case V1:\n"
                   "        break;\n"
                   "    case V2:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Same as above for enum class.
        QTest::newRow("basic1_enum_with_declaration_enumClass")
            << QByteArray(
                   "enum class EnumType { V1, V2 } t;\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    @switch (t) {\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "enum class EnumType { V1, V2 } t;\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    switch (t) {\n"
                   "    case EnumType::V1:\n"
                   "        break;\n"
                   "    case EnumType::V2:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Checks: All enum values are added as case statements for a blank switch
        //         for anonymous enums.
        QTest::newRow("basic1_anonymous_enum")
            << QByteArray(
                   "enum { V1, V2 } t;\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    @switch (t) {\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "enum { V1, V2 } t;\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    switch (t) {\n"
                   "    case V1:\n"
                   "        break;\n"
                   "    case V2:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Checks: All enum values are added as case statements for a blank switch with a default case.
        QTest::newRow("basic2")
            << QByteArray(
                   "enum EnumType { V1, V2 };\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    EnumType t;\n"
                   "    @switch (t) {\n"
                   "    default:\n"
                   "    break;\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "enum EnumType { V1, V2 };\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    EnumType t;\n"
                   "    switch (t) {\n"
                   "    case V1:\n"
                   "        break;\n"
                   "    case V2:\n"
                   "        break;\n"
                   "    default:\n"
                   "    break;\n"
                   "    }\n"
                   "}\n");

        // Same as above for enum class.
        QTest::newRow("basic2_enumClass")
            << QByteArray(
                   "enum class EnumType { V1, V2 };\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    EnumType t;\n"
                   "    @switch (t) {\n"
                   "    default:\n"
                   "    break;\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "enum class EnumType { V1, V2 };\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    EnumType t;\n"
                   "    switch (t) {\n"
                   "    case EnumType::V1:\n"
                   "        break;\n"
                   "    case EnumType::V2:\n"
                   "        break;\n"
                   "    default:\n"
                   "    break;\n"
                   "    }\n"
                   "}\n");

        // Checks: Enum type in class is found.
        QTest::newRow("enumTypeInClass")
            << QByteArray(
                   "struct C { enum EnumType { V1, V2 }; };\n"
                   "\n"
                   "void f(C::EnumType t) {\n"
                   "    @switch (t) {\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "struct C { enum EnumType { V1, V2 }; };\n"
                   "\n"
                   "void f(C::EnumType t) {\n"
                   "    switch (t) {\n"
                   "    case C::V1:\n"
                   "        break;\n"
                   "    case C::V2:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Same as above for enum class.
        QTest::newRow("enumClassInClass")
            << QByteArray(
                   "struct C { enum class EnumType { V1, V2 }; };\n"
                   "\n"
                   "void f(C::EnumType t) {\n"
                   "    @switch (t) {\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "struct C { enum class EnumType { V1, V2 }; };\n"
                   "\n"
                   "void f(C::EnumType t) {\n"
                   "    switch (t) {\n"
                   "    case C::EnumType::V1:\n"
                   "        break;\n"
                   "    case C::EnumType::V2:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Checks: Enum type in namespace is found.
        QTest::newRow("enumTypeInNamespace")
            << QByteArray(
                   "namespace N { enum EnumType { V1, V2 }; };\n"
                   "\n"
                   "void f(N::EnumType t) {\n"
                   "    @switch (t) {\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "namespace N { enum EnumType { V1, V2 }; };\n"
                   "\n"
                   "void f(N::EnumType t) {\n"
                   "    switch (t) {\n"
                   "    case N::V1:\n"
                   "        break;\n"
                   "    case N::V2:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Same as above for enum class.
        QTest::newRow("enumClassInNamespace")
            << QByteArray(
                   "namespace N { enum class EnumType { V1, V2 }; };\n"
                   "\n"
                   "void f(N::EnumType t) {\n"
                   "    @switch (t) {\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "namespace N { enum class EnumType { V1, V2 }; };\n"
                   "\n"
                   "void f(N::EnumType t) {\n"
                   "    switch (t) {\n"
                   "    case N::EnumType::V1:\n"
                   "        break;\n"
                   "    case N::EnumType::V2:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Checks: The missing enum value is added.
        QTest::newRow("oneValueMissing")
            << QByteArray(
                   "enum EnumType { V1, V2 };\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    EnumType t;\n"
                   "    @switch (t) {\n"
                   "    case V2:\n"
                   "        break;\n"
                   "    default:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "enum EnumType { V1, V2 };\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    EnumType t;\n"
                   "    switch (t) {\n"
                   "    case V1:\n"
                   "        break;\n"
                   "    case V2:\n"
                   "        break;\n"
                   "    default:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Checks: Same as above for enum class.
        QTest::newRow("oneValueMissing_enumClass")
            << QByteArray(
                   "enum class EnumType { V1, V2 };\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    EnumType t;\n"
                   "    @switch (t) {\n"
                   "    case EnumType::V2:\n"
                   "        break;\n"
                   "    default:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "enum class EnumType { V1, V2 };\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    EnumType t;\n"
                   "    switch (t) {\n"
                   "    case EnumType::V1:\n"
                   "        break;\n"
                   "    case EnumType::V2:\n"
                   "        break;\n"
                   "    default:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Checks: Find the correct enum type despite there being a declaration with the same name.
        QTest::newRow("QTCREATORBUG10366_1")
            << QByteArray(
                   "enum test { TEST_1, TEST_2 };\n"
                   "\n"
                   "void f() {\n"
                   "    enum test test;\n"
                   "    @switch (test) {\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "enum test { TEST_1, TEST_2 };\n"
                   "\n"
                   "void f() {\n"
                   "    enum test test;\n"
                   "    switch (test) {\n"
                   "    case TEST_1:\n"
                   "        break;\n"
                   "    case TEST_2:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Same as above for enum class.
        QTest::newRow("QTCREATORBUG10366_1_enumClass")
            << QByteArray(
                   "enum class test { TEST_1, TEST_2 };\n"
                   "\n"
                   "void f() {\n"
                   "    enum test test;\n"
                   "    @switch (test) {\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "enum class test { TEST_1, TEST_2 };\n"
                   "\n"
                   "void f() {\n"
                   "    enum test test;\n"
                   "    switch (test) {\n"
                   "    case test::TEST_1:\n"
                   "        break;\n"
                   "    case test::TEST_2:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Checks: Find the correct enum type despite there being a declaration with the same name.
        QTest::newRow("QTCREATORBUG10366_2")
            << QByteArray(
                   "enum test1 { Wrong11, Wrong12 };\n"
                   "enum test { Right1, Right2 };\n"
                   "enum test2 { Wrong21, Wrong22 };\n"
                   "\n"
                   "int main() {\n"
                   "    enum test test;\n"
                   "    @switch (test) {\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "enum test1 { Wrong11, Wrong12 };\n"
                   "enum test { Right1, Right2 };\n"
                   "enum test2 { Wrong21, Wrong22 };\n"
                   "\n"
                   "int main() {\n"
                   "    enum test test;\n"
                   "    switch (test) {\n"
                   "    case Right1:\n"
                   "        break;\n"
                   "    case Right2:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Same as above for enum class.
        QTest::newRow("QTCREATORBUG10366_2_enumClass")
            << QByteArray(
                   "enum class test1 { Wrong11, Wrong12 };\n"
                   "enum class test { Right1, Right2 };\n"
                   "enum class test2 { Wrong21, Wrong22 };\n"
                   "\n"
                   "int main() {\n"
                   "    enum test test;\n"
                   "    @switch (test) {\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "enum class test1 { Wrong11, Wrong12 };\n"
                   "enum class test { Right1, Right2 };\n"
                   "enum class test2 { Wrong21, Wrong22 };\n"
                   "\n"
                   "int main() {\n"
                   "    enum test test;\n"
                   "    switch (test) {\n"
                   "    case test::Right1:\n"
                   "        break;\n"
                   "    case test::Right2:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Checks: Do not crash on incomplete case statetement.
        QTest::newRow("doNotCrashOnIncompleteCase")
            << QByteArray(
                   "enum E {};\n"
                   "void f(E o)\n"
                   "{\n"
                   "    @switch (o)\n"
                   "    {\n"
                   "    case\n"
                   "    }\n"
                   "}\n")
            << QByteArray();

        // Same as above for enum class.
        QTest::newRow("doNotCrashOnIncompleteCase_enumClass")
            << QByteArray(
                   "enum class E {};\n"
                   "void f(E o)\n"
                   "{\n"
                   "    @switch (o)\n"
                   "    {\n"
                   "    case\n"
                   "    }\n"
                   "}\n")
            << QByteArray();

        // Checks: complete switch statement where enum is goes via a template type parameter
        QTest::newRow("QTCREATORBUG-24752")
            << QByteArray(
                   "enum E {EA, EB};\n"
                   "template<typename T> struct S {\n"
                   "    static T theType() { return T(); }\n"
                   "};\n"
                   "int main() {\n"
                   "    @switch (S<E>::theType()) {\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "enum E {EA, EB};\n"
                   "template<typename T> struct S {\n"
                   "    static T theType() { return T(); }\n"
                   "};\n"
                   "int main() {\n"
                   "    switch (S<E>::theType()) {\n"
                   "    case EA:\n"
                   "        break;\n"
                   "    case EB:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Same as above for enum class.
        QTest::newRow("QTCREATORBUG-24752_enumClass")
            << QByteArray(
                   "enum class E {A, B};\n"
                   "template<typename T> struct S {\n"
                   "    static T theType() { return T(); }\n"
                   "};\n"
                   "int main() {\n"
                   "    @switch (S<E>::theType()) {\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "enum class E {A, B};\n"
                   "template<typename T> struct S {\n"
                   "    static T theType() { return T(); }\n"
                   "};\n"
                   "int main() {\n"
                   "    switch (S<E>::theType()) {\n"
                   "    case E::A:\n"
                   "        break;\n"
                   "    case E::B:\n"
                   "        break;\n"
                   "    }\n"
                   "}\n");

        // Checks: Complete switch statement where enum is return type of a template function
        //         which is outside the scope of the return value.
        // TODO: Type minimization.
        QTest::newRow("QTCREATORBUG-25998")
            << QByteArray(
                   "template <typename T> T enumCast(int value) { return static_cast<T>(value); }\n"
                   "class Test {\n"
                   "    enum class E { V1, V2 };"
                   "    void func(int i) {\n"
                   "        @switch (enumCast<E>(i)) {\n"
                   "        }\n"
                   "    }\n"
                   "};\n")
            << QByteArray(
                   "template <typename T> T enumCast(int value) { return static_cast<T>(value); }\n"
                   "class Test {\n"
                   "    enum class E { V1, V2 };"
                   "    void func(int i) {\n"
                   "        switch (enumCast<E>(i)) {\n"
                   "        case Test::E::V1:\n"
                   "            break;\n"
                   "        case Test::E::V2:\n"
                   "            break;\n"
                   "        }\n"
                   "    }\n"
                   "};\n");
    }

    void test()
    {
        QFETCH(QByteArray, original);
        QFETCH(QByteArray, expected);
        CompleteSwitchStatement factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }
};

QObject *CompleteSwitchStatement::createTest() { return new CompleteSwitchStatementTest; }

#endif // WITH_TESTS
} // namespace

void registerCompleteSwitchStatementQuickfix()
{
    CppQuickFixFactory::registerFactory<CompleteSwitchStatement>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <completeswitchstatement.moc>
#endif
