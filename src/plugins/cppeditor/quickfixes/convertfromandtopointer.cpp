// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "convertfromandtopointer.h"

#include "../cppeditortr.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"

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

class ConvertFromAndToPointerOp : public CppQuickFixOperation
{
public:
    enum Mode { FromPointer, FromVariable, FromReference };

    ConvertFromAndToPointerOp(const CppQuickFixInterface &interface, int priority, Mode mode,
                              bool isAutoDeclaration,
                              const SimpleDeclarationAST *simpleDeclaration,
                              const DeclaratorAST *declaratorAST,
                              const SimpleNameAST *identifierAST,
                              Symbol *symbol)
        : CppQuickFixOperation(interface, priority)
        , m_mode(mode)
        , m_isAutoDeclaration(isAutoDeclaration)
        , m_simpleDeclaration(simpleDeclaration)
        , m_declaratorAST(declaratorAST)
        , m_identifierAST(identifierAST)
        , m_symbol(symbol)
        , m_refactoring(snapshot())
        , m_file(m_refactoring.cppFile(filePath()))
        , m_document(interface.semanticInfo().doc)
    {
        setDescription(
            mode == FromPointer
                ? Tr::tr("Convert to Stack Variable")
                : Tr::tr("Convert to Pointer"));
    }

    void perform() override
    {
        ChangeSet changes;

        switch (m_mode) {
        case FromPointer:
            removePointerOperator(changes);
            convertToStackVariable(changes);
            break;
        case FromReference:
            removeReferenceOperator(changes);
            Q_FALLTHROUGH();
        case FromVariable:
            convertToPointer(changes);
            break;
        }

        m_file->setChangeSet(changes);
        m_file->apply();
    }

private:
    void removePointerOperator(ChangeSet &changes) const
    {
        if (!m_declaratorAST->ptr_operator_list)
            return;
        PointerAST *ptrAST = m_declaratorAST->ptr_operator_list->value->asPointer();
        QTC_ASSERT(ptrAST, return);
        const int pos = m_file->startOf(ptrAST->star_token);
        changes.remove(pos, pos + 1);
    }

    void removeReferenceOperator(ChangeSet &changes) const
    {
        ReferenceAST *refAST = m_declaratorAST->ptr_operator_list->value->asReference();
        QTC_ASSERT(refAST, return);
        const int pos = m_file->startOf(refAST->reference_token);
        changes.remove(pos, pos + 1);
    }

    void removeNewExpression(ChangeSet &changes, NewExpressionAST *newExprAST) const
    {
        ExpressionListAST *exprlist = nullptr;
        if (newExprAST->new_initializer) {
            if (ExpressionListParenAST *ast = newExprAST->new_initializer->asExpressionListParen())
                exprlist = ast->expression_list;
            else if (BracedInitializerAST *ast = newExprAST->new_initializer->asBracedInitializer())
                exprlist = ast->expression_list;
        }

        if (exprlist) {
            // remove 'new' keyword and type before initializer
            changes.remove(m_file->startOf(newExprAST->new_token),
                           m_file->startOf(newExprAST->new_initializer));

            changes.remove(m_file->endOf(m_declaratorAST->equal_token - 1),
                           m_file->startOf(m_declaratorAST->equal_token + 1));
        } else {
            // remove the whole new expression
            changes.remove(m_file->endOf(m_identifierAST->firstToken()),
                           m_file->startOf(newExprAST->lastToken()));
        }
    }

    void removeNewKeyword(ChangeSet &changes, NewExpressionAST *newExprAST) const
    {
        // remove 'new' keyword before initializer
        changes.remove(m_file->startOf(newExprAST->new_token),
                       m_file->startOf(newExprAST->new_type_id));
    }

    void convertToStackVariable(ChangeSet &changes) const
    {
        // Handle the initializer.
        if (m_declaratorAST->initializer) {
            if (NewExpressionAST *newExpression = m_declaratorAST->initializer->asNewExpression()) {
                if (m_isAutoDeclaration) {
                    if (!newExpression->new_initializer)
                        changes.insert(m_file->endOf(newExpression), QStringLiteral("()"));
                    removeNewKeyword(changes, newExpression);
                } else {
                    removeNewExpression(changes, newExpression);
                }
            }
        }

        // Fix all occurrences of the identifier in this function.
        ASTPath astPath(m_document);
        const QList<SemanticInfo::Use> uses = semanticInfo().localUses.value(m_symbol);
        for (const SemanticInfo::Use &use : uses) {
            const QList<AST *> path = astPath(use.line, use.column);
            AST *idAST = path.last();
            bool declarationFound = false;
            bool starFound = false;
            int ampersandPos = 0;
            bool memberAccess = false;
            bool deleteCall = false;

            for (int i = path.count() - 2; i >= 0; --i) {
                if (path.at(i) == m_declaratorAST) {
                    declarationFound = true;
                    break;
                }
                if (MemberAccessAST *memberAccessAST = path.at(i)->asMemberAccess()) {
                    if (m_file->tokenAt(memberAccessAST->access_token).kind() != T_ARROW)
                        continue;
                    int pos = m_file->startOf(memberAccessAST->access_token);
                    changes.replace(pos, pos + 2, QLatin1String("."));
                    memberAccess = true;
                    break;
                } else if (DeleteExpressionAST *deleteAST = path.at(i)->asDeleteExpression()) {
                    const int pos = m_file->startOf(deleteAST->delete_token);
                    changes.insert(pos, QLatin1String("// "));
                    deleteCall = true;
                    break;
                } else if (UnaryExpressionAST *unaryExprAST = path.at(i)->asUnaryExpression()) {
                    const Token tk = m_file->tokenAt(unaryExprAST->unary_op_token);
                    if (tk.kind() == T_STAR) {
                        if (!starFound) {
                            int pos = m_file->startOf(unaryExprAST->unary_op_token);
                            changes.remove(pos, pos + 1);
                        }
                        starFound = true;
                    } else if (tk.kind() == T_AMPER) {
                        ampersandPos = m_file->startOf(unaryExprAST->unary_op_token);
                    }
                } else if (PointerAST *ptrAST = path.at(i)->asPointer()) {
                    if (!starFound) {
                        const int pos = m_file->startOf(ptrAST->star_token);
                        changes.remove(pos, pos);
                    }
                    starFound = true;
                } else if (path.at(i)->asFunctionDefinition()) {
                    break;
                }
            }
            if (!declarationFound && !starFound && !memberAccess && !deleteCall) {
                if (ampersandPos) {
                    changes.insert(ampersandPos, QLatin1String("&("));
                    changes.insert(m_file->endOf(idAST->firstToken()), QLatin1String(")"));
                } else {
                    changes.insert(m_file->startOf(idAST), QLatin1String("&"));
                }
            }
        }
    }

    QString typeNameOfDeclaration() const
    {
        if (!m_simpleDeclaration
            || !m_simpleDeclaration->decl_specifier_list
            || !m_simpleDeclaration->decl_specifier_list->value) {
            return QString();
        }
        NamedTypeSpecifierAST *namedType
            = m_simpleDeclaration->decl_specifier_list->value->asNamedTypeSpecifier();
        if (!namedType)
            return QString();

        Overview overview;
        return overview.prettyName(namedType->name->name);
    }

    void insertNewExpression(ChangeSet &changes, ExpressionAST *ast) const
    {
        const QString typeName = typeNameOfDeclaration();
        if (CallAST *callAST = ast->asCall()) {
            if (typeName.isEmpty()) {
                changes.insert(m_file->startOf(callAST), QLatin1String("new "));
            } else {
                changes.insert(m_file->startOf(callAST),
                               QLatin1String("new ") + typeName + QLatin1Char('('));
                changes.insert(m_file->startOf(callAST->lastToken()), QLatin1String(")"));
            }
        } else {
            if (typeName.isEmpty())
                return;
            changes.insert(m_file->startOf(ast), QLatin1String(" = new ") + typeName);
        }
    }

    void insertNewExpression(ChangeSet &changes) const
    {
        const QString typeName = typeNameOfDeclaration();
        if (typeName.isEmpty())
            return;
        changes.insert(m_file->endOf(m_identifierAST->firstToken()),
                       QLatin1String(" = new ") + typeName);
    }

    void convertToPointer(ChangeSet &changes) const
    {
        // Handle initializer.
        if (m_declaratorAST->initializer) {
            if (IdExpressionAST *idExprAST = m_declaratorAST->initializer->asIdExpression()) {
                changes.insert(m_file->startOf(idExprAST), QLatin1String("&"));
            } else if (CallAST *callAST = m_declaratorAST->initializer->asCall()) {
                insertNewExpression(changes, callAST);
            } else if (ExpressionListParenAST *exprListAST = m_declaratorAST->initializer
                                                                 ->asExpressionListParen()) {
                insertNewExpression(changes, exprListAST);
            } else if (BracedInitializerAST *bracedInitializerAST = m_declaratorAST->initializer
                                                                        ->asBracedInitializer()) {
                insertNewExpression(changes, bracedInitializerAST);
            }
        } else {
            insertNewExpression(changes);
        }

        // Fix all occurrences of the identifier in this function.
        ASTPath astPath(m_document);
        const QList<SemanticInfo::Use> uses = semanticInfo().localUses.value(m_symbol);
        for (const SemanticInfo::Use &use : uses) {
            const QList<AST *> path = astPath(use.line, use.column);
            AST *idAST = path.last();
            bool insertStar = true;
            for (int i = path.count() - 2; i >= 0; --i) {
                if (m_isAutoDeclaration && path.at(i) == m_declaratorAST) {
                    insertStar = false;
                    break;
                }
                if (MemberAccessAST *memberAccessAST = path.at(i)->asMemberAccess()) {
                    const int pos = m_file->startOf(memberAccessAST->access_token);
                    changes.replace(pos, pos + 1, QLatin1String("->"));
                    insertStar = false;
                    break;
                } else if (UnaryExpressionAST *unaryExprAST = path.at(i)->asUnaryExpression()) {
                    if (m_file->tokenAt(unaryExprAST->unary_op_token).kind() == T_AMPER) {
                        const int pos = m_file->startOf(unaryExprAST->unary_op_token);
                        changes.remove(pos, pos + 1);
                        insertStar = false;
                        break;
                    }
                } else if (path.at(i)->asFunctionDefinition()) {
                    break;
                }
            }
            if (insertStar)
                changes.insert(m_file->startOf(idAST), QLatin1String("*"));
        }
    }

    const Mode m_mode;
    const bool m_isAutoDeclaration;
    const SimpleDeclarationAST * const m_simpleDeclaration;
    const DeclaratorAST * const m_declaratorAST;
    const SimpleNameAST * const m_identifierAST;
    Symbol * const m_symbol;
    const CppRefactoringChanges m_refactoring;
    const CppRefactoringFilePtr m_file;
    const Document::Ptr m_document;
};

/*!
  Converts the selected variable to a pointer if it is a stack variable or reference, or vice versa.
  Activates on variable declarations.
 */
class ConvertFromAndToPointer : public CppQuickFixFactory
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
        SimpleNameAST *identifier = path.last()->asSimpleName();
        if (!identifier)
            return;
        SimpleDeclarationAST *simpleDeclaration = nullptr;
        DeclaratorAST *declarator = nullptr;
        bool isFunctionLocal = false;
        bool isClassLocal = false;
        ConvertFromAndToPointerOp::Mode mode = ConvertFromAndToPointerOp::FromVariable;
        for (int i = path.count() - 2; i >= 0; --i) {
            AST *ast = path.at(i);
            if (!declarator && (declarator = ast->asDeclarator()))
                continue;
            if (!simpleDeclaration && (simpleDeclaration = ast->asSimpleDeclaration()))
                continue;
            if (declarator && simpleDeclaration) {
                if (ast->asClassSpecifier()) {
                    isClassLocal = true;
                } else if (ast->asFunctionDefinition() && !isClassLocal) {
                    isFunctionLocal = true;
                    break;
                }
            }
        }
        if (!isFunctionLocal || !simpleDeclaration || !declarator)
            return;

        Symbol *symbol = nullptr;
        for (List<Symbol *> *lst = simpleDeclaration->symbols; lst; lst = lst->next) {
            if (lst->value->name() == identifier->name) {
                symbol = lst->value;
                break;
            }
        }
        if (!symbol)
            return;

        bool isAutoDeclaration = false;
        if (symbol->storage() == Symbol::Auto) {
            // For auto variables we must deduce the type from the initializer.
            if (!declarator->initializer)
                return;

            isAutoDeclaration = true;
            TypeOfExpression typeOfExpression;
            typeOfExpression.init(interface.semanticInfo().doc, interface.snapshot());
            typeOfExpression.setExpandTemplates(true);
            CppRefactoringFilePtr file = interface.currentFile();
            Scope *scope = file->scopeAt(declarator->firstToken());
            QList<LookupItem> result = typeOfExpression(file->textOf(declarator->initializer).toUtf8(),
                                                        scope, TypeOfExpression::Preprocess);
            if (!result.isEmpty() && result.first().type()->asPointerType())
                mode = ConvertFromAndToPointerOp::FromPointer;
        } else if (declarator->ptr_operator_list) {
            for (PtrOperatorListAST *ops = declarator->ptr_operator_list; ops; ops = ops->next) {
                if (ops != declarator->ptr_operator_list) {
                    // Bail out on more complex pointer types (e.g. pointer of pointer,
                    // or reference of pointer).
                    return;
                }
                if (ops->value->asPointer())
                    mode = ConvertFromAndToPointerOp::FromPointer;
                else if (ops->value->asReference())
                    mode = ConvertFromAndToPointerOp::FromReference;
            }
        }

        const int priority = path.size() - 1;
        result << new ConvertFromAndToPointerOp(interface, priority, mode, isAutoDeclaration,
                                                simpleDeclaration, declarator, identifier, symbol);
    }
};

#ifdef WITH_TESTS
using namespace Tests;

class ConvertFromAndToPointerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArray>("original");
        QTest::addColumn<QByteArray>("expected");

        QTest::newRow("ConvertFromPointer")
            << QByteArray("void foo() {\n"
                 "    QString *@str;\n"
                 "    if (!str->isEmpty())\n"
                 "        str->clear();\n"
                 "    f1(*str);\n"
                 "    f2(str);\n"
                 "}\n")
            << QByteArray("void foo() {\n"
                 "    QString str;\n"
                 "    if (!str.isEmpty())\n"
                 "        str.clear();\n"
                 "    f1(str);\n"
                 "    f2(&str);\n"
                 "}\n");

        QTest::newRow("ConvertToPointer")
            << QByteArray("void foo() {\n"
                 "    QString @str;\n"
                 "    if (!str.isEmpty())\n"
                 "        str.clear();\n"
                 "    f1(str);\n"
                 "    f2(&str);\n"
                 "}\n")
            << QByteArray("void foo() {\n"
                 "    QString *str = new QString;\n"
                 "    if (!str->isEmpty())\n"
                 "        str->clear();\n"
                 "    f1(*str);\n"
                 "    f2(str);\n"
                 "}\n");

        QTest::newRow("ConvertReferenceToPointer")
            << QByteArray("void foo() {\n"
                 "    QString narf;"
                 "    QString &@str = narf;\n"
                 "    if (!str.isEmpty())\n"
                 "        str.clear();\n"
                 "    f1(str);\n"
                 "    f2(&str);\n"
                 "}\n")
            << QByteArray("void foo() {\n"
                 "    QString narf;"
                 "    QString *str = &narf;\n"
                 "    if (!str->isEmpty())\n"
                 "        str->clear();\n"
                 "    f1(*str);\n"
                 "    f2(str);\n"
                 "}\n");

        QTest::newRow("ConvertFromPointer_withInitializer")
            << QByteArray("void foo() {\n"
                 "    QString *@str = new QString(QLatin1String(\"schnurz\"));\n"
                 "    if (!str->isEmpty())\n"
                 "        str->clear();\n"
                 "}\n")
            << QByteArray("void foo() {\n"
                 "    QString str(QLatin1String(\"schnurz\"));\n"
                 "    if (!str.isEmpty())\n"
                 "        str.clear();\n"
                 "}\n");

        QTest::newRow("ConvertFromPointer_withBareInitializer")
            << QByteArray("void foo() {\n"
                 "    QString *@str = new QString;\n"
                 "    if (!str->isEmpty())\n"
                 "        str->clear();\n"
                 "}\n")
            << QByteArray("void foo() {\n"
                 "    QString str;\n"
                 "    if (!str.isEmpty())\n"
                 "        str.clear();\n"
                 "}\n");

        QTest::newRow("ConvertFromPointer_withEmptyInitializer")
            << QByteArray("void foo() {\n"
                 "    QString *@str = new QString();\n"
                 "    if (!str->isEmpty())\n"
                 "        str->clear();\n"
                 "}\n")
            << QByteArray("void foo() {\n"
                 "    QString str;\n"
                 "    if (!str.isEmpty())\n"
                 "        str.clear();\n"
                 "}\n");

        QTest::newRow("ConvertFromPointer_structWithPointer")
            << QByteArray("struct Bar{ QString *str; };\n"
                 "void foo() {\n"
                 "    Bar *@bar = new Bar;\n"
                 "    bar->str = new QString;\n"
                 "    delete bar->str;\n"
                 "    delete bar;\n"
                 "}\n")
            << QByteArray("struct Bar{ QString *str; };\n"
                 "void foo() {\n"
                 "    Bar bar;\n"
                 "    bar.str = new QString;\n"
                 "    delete bar.str;\n"
                 "    // delete bar;\n"
                 "}\n");

        QTest::newRow("ConvertToPointer_withInitializer")
            << QByteArray("void foo() {\n"
                 "    QString @str = QLatin1String(\"narf\");\n"
                 "    if (!str.isEmpty())\n"
                 "        str.clear();\n"
                 "}\n")
            << QByteArray("void foo() {\n"
                 "    QString *str = new QString(QLatin1String(\"narf\"));\n"
                 "    if (!str->isEmpty())\n"
                 "        str->clear();\n"
                 "}\n");

        QTest::newRow("ConvertToPointer_withParenInitializer")
            << QByteArray("void foo() {\n"
                 "    QString @str(QLatin1String(\"narf\"));\n"
                 "    if (!str.isEmpty())\n"
                 "        str.clear();\n"
                 "}\n")
            << QByteArray("void foo() {\n"
                 "    QString *str = new QString(QLatin1String(\"narf\"));\n"
                 "    if (!str->isEmpty())\n"
                 "        str->clear();\n"
                 "}\n");

        QTest::newRow("ConvertToPointer_noTriggerRValueRefs")
            << QByteArray("void foo(Narf &&@narf) {}\n")
            << QByteArray();

        QTest::newRow("ConvertToPointer_noTriggerGlobal")
            << QByteArray("int @global;\n")
            << QByteArray();

        QTest::newRow("ConvertToPointer_noTriggerClassMember")
            << QByteArray("struct C { int @member; };\n")
            << QByteArray();

        QTest::newRow("ConvertToPointer_noTriggerClassMember2")
            << QByteArray("void f() { struct C { int @member; }; }\n")
            << QByteArray();

        QTest::newRow("ConvertToPointer_functionOfFunctionLocalClass")
            << QByteArray("void f() {\n"
                 "    struct C {\n"
                 "        void g() { int @member; }\n"
                 "    };\n"
                 "}\n")
            << QByteArray("void f() {\n"
                 "    struct C {\n"
                 "        void g() { int *member; }\n"
                 "    };\n"
                 "}\n");

        QTest::newRow("ConvertToPointer_redeclaredVariable_block")
            << QByteArray("void foo() {\n"
                 "    QString @str;\n"
                 "    str.clear();\n"
                 "    {\n"
                 "        QString str;\n"
                 "        str.clear();\n"
                 "    }\n"
                 "    f1(str);\n"
                 "}\n")
            << QByteArray("void foo() {\n"
                 "    QString *str = new QString;\n"
                 "    str->clear();\n"
                 "    {\n"
                 "        QString str;\n"
                 "        str.clear();\n"
                 "    }\n"
                 "    f1(*str);\n"
                 "}\n");

        QTest::newRow("ConvertAutoFromPointer")
            << QByteArray("void foo() {\n"
                 "    auto @str = new QString(QLatin1String(\"foo\"));\n"
                 "    if (!str->isEmpty())\n"
                 "        str->clear();\n"
                 "    f1(*str);\n"
                 "    f2(str);\n"
                 "}\n")
            << QByteArray("void foo() {\n"
                 "    auto str = QString(QLatin1String(\"foo\"));\n"
                 "    if (!str.isEmpty())\n"
                 "        str.clear();\n"
                 "    f1(str);\n"
                 "    f2(&str);\n"
                 "}\n");

        QTest::newRow("ConvertAutoFromPointer2")
            << QByteArray("void foo() {\n"
                 "    auto *@str = new QString;\n"
                 "    if (!str->isEmpty())\n"
                 "        str->clear();\n"
                 "    f1(*str);\n"
                 "    f2(str);\n"
                 "}\n")
            << QByteArray("void foo() {\n"
                 "    auto str = QString();\n"
                 "    if (!str.isEmpty())\n"
                 "        str.clear();\n"
                 "    f1(str);\n"
                 "    f2(&str);\n"
                 "}\n");

        QTest::newRow("ConvertAutoToPointer")
            << QByteArray("void foo() {\n"
                 "    auto @str = QString(QLatin1String(\"foo\"));\n"
                 "    if (!str.isEmpty())\n"
                 "        str.clear();\n"
                 "    f1(str);\n"
                 "    f2(&str);\n"
                 "}\n")
            << QByteArray("void foo() {\n"
                 "    auto @str = new QString(QLatin1String(\"foo\"));\n"
                 "    if (!str->isEmpty())\n"
                 "        str->clear();\n"
                 "    f1(*str);\n"
                 "    f2(str);\n"
                 "}\n");

        QTest::newRow("ConvertToPointerWithMacro")
            << QByteArray("#define BAR bar\n"
                 "void func()\n"
                 "{\n"
                 "    int @foo = 42;\n"
                 "    int bar;\n"
                 "    BAR = foo;\n"
                 "}\n")
            << QByteArray("#define BAR bar\n"
                 "void func()\n"
                 "{\n"
                 "    int *foo = 42;\n"
                 "    int bar;\n"
                 "    BAR = *foo;\n"
                 "}\n");

        QString testObjAndFunc = "struct Object\n"
                                 "{\n"
                                 "    Object(%1){}\n"
                                 "};\n"
                                 "void func()\n"
                                 "{\n"
                                 "    %2\n"
                                 "}\n";

        QTest::newRow("ConvertToStack1_QTCREATORBUG23181")
            << QByteArray(testObjAndFunc.arg("int").arg("Object *@obj = new Object(0);").toUtf8())
            << QByteArray(testObjAndFunc.arg("int").arg("Object obj(0);").toUtf8());

        QTest::newRow("ConvertToStack2_QTCREATORBUG23181")
            << QByteArray(testObjAndFunc.arg("int").arg("Object *@obj = new Object{0};").toUtf8())
            << QByteArray(testObjAndFunc.arg("int").arg("Object obj{0};").toUtf8());

        QTest::newRow("ConvertToPointer1_QTCREATORBUG23181")
            << QByteArray(testObjAndFunc.arg("").arg("Object @obj;").toUtf8())
            << QByteArray(testObjAndFunc.arg("").arg("Object *obj = new Object;").toUtf8());

        QTest::newRow("ConvertToPointer2_QTCREATORBUG23181")
            << QByteArray(testObjAndFunc.arg("").arg("Object @obj();").toUtf8())
            << QByteArray(testObjAndFunc.arg("").arg("Object *obj = new Object();").toUtf8());

        QTest::newRow("ConvertToPointer3_QTCREATORBUG23181")
            << QByteArray(testObjAndFunc.arg("").arg("Object @obj{};").toUtf8())
            << QByteArray(testObjAndFunc.arg("").arg("Object *obj = new Object{};").toUtf8());

        QTest::newRow("ConvertToPointer4_QTCREATORBUG23181")
            << QByteArray(testObjAndFunc.arg("int").arg("Object @obj(0);").toUtf8())
            << QByteArray(testObjAndFunc.arg("int").arg("Object *obj = new Object(0);").toUtf8());


    }

    void test()
    {
        QFETCH(QByteArray, original);
        QFETCH(QByteArray, expected);
        ConvertFromAndToPointer factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }
};

QObject *ConvertFromAndToPointer::createTest() { return new ConvertFromAndToPointerTest; }

#endif // WITH_TESTS
} // namespace

void registerConvertFromAndToPointerQuickfix()
{
    CppQuickFixFactory::registerFactory<ConvertFromAndToPointer>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <convertfromandtopointer.moc>
#endif
