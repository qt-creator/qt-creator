// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "assigntolocalvariable.h"

#include "../cppcodestylesettings.h"
#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"
#include "cppquickfixprojectsettings.h"

#include <cplusplus/CppRewriter.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <projectexplorer/projecttree.h>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#include <QtTest>
#endif

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

class AssignToLocalVariableOperation : public CppQuickFixOperation
{
public:
    explicit AssignToLocalVariableOperation(const CppQuickFixInterface &interface,
                                            const int insertPos, const AST *ast, const Name *name)
        : CppQuickFixOperation(interface)
        , m_insertPos(insertPos)
        , m_ast(ast)
        , m_name(name)
        , m_oo(CppCodeStyleSettings::currentProjectCodeStyleOverview())
        , m_originalName(m_oo.prettyName(m_name))
        , m_file(interface.currentFile())
    {
        setDescription(Tr::tr("Assign to Local Variable"));
    }

private:
    void perform() override
    {
        QString type = deduceType();
        if (type.isEmpty())
            return;
        const int origNameLength = m_originalName.length();
        const QString varName = constructVarName();
        const QString insertString = type.replace(type.length() - origNameLength, origNameLength,
                                                  varName + QLatin1String(" = "));
        ChangeSet changes;
        changes.insert(m_insertPos, insertString);
        m_file->apply(changes);

        // move cursor to new variable name
        QTextCursor c = m_file->cursor();
        c.setPosition(m_insertPos + insertString.length() - varName.length() - 3);
        c.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        editor()->setTextCursor(c);
    }

    QString deduceType() const
    {
        const auto settings = CppQuickFixProjectsSettings::getQuickFixSettings(
            ProjectExplorer::ProjectTree::currentProject());
        if (m_file->cppDocument()->languageFeatures().cxx11Enabled && settings->useAuto)
            return "auto " + m_originalName;

        TypeOfExpression typeOfExpression;
        typeOfExpression.init(semanticInfo().doc, snapshot(), context().bindings());
        typeOfExpression.setExpandTemplates(true);
        Scope * const scope = m_file->scopeAt(m_ast->firstToken());
        const QList<LookupItem> result = typeOfExpression(m_file->textOf(m_ast).toUtf8(),
                                                          scope, TypeOfExpression::Preprocess);
        if (result.isEmpty())
            return {};

        SubstitutionEnvironment env;
        env.setContext(context());
        env.switchScope(result.first().scope());
        ClassOrNamespace *con = typeOfExpression.context().lookupType(scope);
        if (!con)
            con = typeOfExpression.context().globalNamespace();
        UseMinimalNames q(con);
        env.enter(&q);

        Control *control = context().bindings()->control().get();
        FullySpecifiedType type = rewriteType(result.first().type(), &env, control);

        return m_oo.prettyType(type, m_name);
    }

    QString constructVarName() const
    {
        QString newName = m_originalName;
        if (newName.startsWith(QLatin1String("get"), Qt::CaseInsensitive)
            && newName.length() > 3
            && newName.at(3).isUpper()) {
            newName.remove(0, 3);
            newName.replace(0, 1, newName.at(0).toLower());
        } else if (newName.startsWith(QLatin1String("to"), Qt::CaseInsensitive)
                   && newName.length() > 2
                   && newName.at(2).isUpper()) {
            newName.remove(0, 2);
            newName.replace(0, 1, newName.at(0).toLower());
        } else {
            newName.replace(0, 1, newName.at(0).toUpper());
            newName.prepend(QLatin1String("local"));
        }
        return newName;
    }

    const int m_insertPos;
    const AST * const m_ast;
    const Name * const m_name;
    const Overview m_oo;
    const QString m_originalName;
    const CppRefactoringFilePtr m_file;
};

//! Assigns the return value of a function call or a new expression to a local variable
class AssignToLocalVariable : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        AST *outerAST = nullptr;
        SimpleNameAST *nameAST = nullptr;

        for (int i = path.size() - 3; i >= 0; --i) {
            if (CallAST *callAST = path.at(i)->asCall()) {
                if (!interface.isCursorOn(callAST))
                    return;
                if (i - 2 >= 0) {
                    const int idx = i - 2;
                    if (path.at(idx)->asSimpleDeclaration())
                        return;
                    if (path.at(idx)->asExpressionStatement())
                        return;
                    if (path.at(idx)->asMemInitializer())
                        return;
                    if (path.at(idx)->asCall()) { // Fallback if we have a->b()->c()...
                        --i;
                        continue;
                    }
                }
                for (int a = i - 1; a > 0; --a) {
                    if (path.at(a)->asBinaryExpression())
                        return;
                    if (path.at(a)->asReturnStatement())
                        return;
                    if (path.at(a)->asCall())
                        return;
                }

                if (MemberAccessAST *member = path.at(i + 1)->asMemberAccess()) { // member
                    if (NameAST *name = member->member_name)
                        nameAST = name->asSimpleName();
                } else if (QualifiedNameAST *qname = path.at(i + 2)->asQualifiedName()) { // static or
                    nameAST = qname->unqualified_name->asSimpleName();                    // func in ns
                } else { // normal
                    nameAST = path.at(i + 2)->asSimpleName();
                }

                if (nameAST) {
                    outerAST = callAST;
                    break;
                }
            } else if (NewExpressionAST *newexp = path.at(i)->asNewExpression()) {
                if (!interface.isCursorOn(newexp))
                    return;
                if (i - 2 >= 0) {
                    const int idx = i - 2;
                    if (path.at(idx)->asSimpleDeclaration())
                        return;
                    if (path.at(idx)->asExpressionStatement())
                        return;
                    if (path.at(idx)->asMemInitializer())
                        return;
                }
                for (int a = i - 1; a > 0; --a) {
                    if (path.at(a)->asReturnStatement())
                        return;
                    if (path.at(a)->asCall())
                        return;
                }

                if (NamedTypeSpecifierAST *ts = path.at(i + 2)->asNamedTypeSpecifier()) {
                    nameAST = ts->name->asSimpleName();
                    outerAST = newexp;
                    break;
                }
            }
        }

        if (outerAST && nameAST) {
            const CppRefactoringFilePtr file = interface.currentFile();
            QList<LookupItem> items;
            TypeOfExpression typeOfExpression;
            typeOfExpression.init(interface.semanticInfo().doc, interface.snapshot(),
                                  interface.context().bindings());
            typeOfExpression.setExpandTemplates(true);

            // If items are empty, AssignToLocalVariableOperation will fail.
            items = typeOfExpression(file->textOf(outerAST).toUtf8(),
                                     file->scopeAt(outerAST->firstToken()),
                                     TypeOfExpression::Preprocess);
            if (items.isEmpty())
                return;

            if (CallAST *callAST = outerAST->asCall()) {
                items = typeOfExpression(file->textOf(callAST->base_expression).toUtf8(),
                                         file->scopeAt(callAST->base_expression->firstToken()),
                                         TypeOfExpression::Preprocess);
            } else {
                items = typeOfExpression(file->textOf(nameAST).toUtf8(),
                                         file->scopeAt(nameAST->firstToken()),
                                         TypeOfExpression::Preprocess);
            }

            for (const LookupItem &item : std::as_const(items)) {
                if (!item.declaration())
                    continue;

                if (Function *func = item.declaration()->asFunction()) {
                    if (func->isSignal() || func->returnType()->asVoidType())
                        return;
                } else if (Declaration *dec = item.declaration()->asDeclaration()) {
                    if (Function *func = dec->type()->asFunctionType()) {
                        if (func->isSignal() || func->returnType()->asVoidType())
                            return;
                    }
                }

                const Name *name = nameAST->name;
                const int insertPos = interface.currentFile()->startOf(outerAST);
                result << new AssignToLocalVariableOperation(interface, insertPos, outerAST, name);
                return;
            }
        }
    }
};

#ifdef WITH_TESTS
using namespace Tests;

class AssignToLocalVariableTest : public QObject
{
    Q_OBJECT

private slots:
    void testTemplates()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "template <typename T>\n"
            "class List {\n"
            "public:\n"
            "    T first();"
            "};\n"
            ;
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n"
            "void foo() {\n"
            "    List<int> list;\n"
            "    li@st.first();\n"
            "}\n";
        expected =
            "#include \"file.h\"\n"
            "void foo() {\n"
            "    List<int> list;\n"
            "    auto localFirst = list.first();\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        AssignToLocalVariable factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void test_data()
    {
        QTest::addColumn<QByteArray>("original");
        QTest::addColumn<QByteArray>("expected");

        // Check: Add local variable for a free function.
        QTest::newRow("freeFunction")
            << QByteArray(
                   "int foo() {return 1;}\n"
                   "void bar() {fo@o();}\n")
            << QByteArray(
                   "int foo() {return 1;}\n"
                   "void bar() {auto localFoo = foo();}\n");

        // Check: Add local variable for a member function.
        QTest::newRow("memberFunction")
            << QByteArray(
                   "class Foo {public: int* fooFunc();}\n"
                   "void bar() {\n"
                   "    Foo *f = new Foo;\n"
                   "    @f->fooFunc();\n"
                   "}\n")
            << QByteArray(
                   "class Foo {public: int* fooFunc();}\n"
                   "void bar() {\n"
                   "    Foo *f = new Foo;\n"
                   "    auto localFooFunc = f->fooFunc();\n"
                   "}\n");

        // Check: Add local variable for a member function, cursor in the middle (QTCREATORBUG-10355)
        QTest::newRow("memberFunction2ndGrade1")
            << QByteArray(
                   "struct Foo {int* func();};\n"
                   "struct Baz {Foo* foo();};\n"
                   "void bar() {\n"
                   "    Baz *b = new Baz;\n"
                   "    b->foo@()->func();\n"
                   "}")
            << QByteArray(
                   "struct Foo {int* func();};\n"
                   "struct Baz {Foo* foo();};\n"
                   "void bar() {\n"
                   "    Baz *b = new Baz;\n"
                   "    auto localFunc = b->foo()->func();\n"
                   "}");

        // Check: Add local variable for a member function, cursor on function call (QTCREATORBUG-10355)
        QTest::newRow("memberFunction2ndGrade2")
            << QByteArray(
                   "struct Foo {int* func();};\n"
                   "struct Baz {Foo* foo();};\n"
                   "void bar() {\n"
                   "    Baz *b = new Baz;\n"
                   "    b->foo()->f@unc();\n"
                   "}")
            << QByteArray(
                   "struct Foo {int* func();};\n"
                   "struct Baz {Foo* foo();};\n"
                   "void bar() {\n"
                   "    Baz *b = new Baz;\n"
                   "    auto localFunc = b->foo()->func();\n"
                   "}");

        // Check: Add local variable for a static member function.
        QTest::newRow("staticMemberFunction")
            << QByteArray(
                   "class Foo {public: static int* fooFunc();}\n"
                   "void bar() {\n"
                   "    Foo::fooF@unc();\n"
                   "}")
            << QByteArray(
                   "class Foo {public: static int* fooFunc();}\n"
                   "void bar() {\n"
                   "    auto localFooFunc = Foo::fooFunc();\n"
                   "}");

        // Check: Add local variable for a new Expression.
        QTest::newRow("newExpression")
            << QByteArray(
                   "class Foo {}\n"
                   "void bar() {\n"
                   "    new Fo@o;\n"
                   "}")
            << QByteArray(
                   "class Foo {}\n"
                   "void bar() {\n"
                   "    auto localFoo = new Foo;\n"
                   "}");

        // Check: No trigger for function inside member initialization list.
        QTest::newRow("noInitializationList")
            << QByteArray(
                   "class Foo\n"
                   "{\n"
                   "    public: Foo : m_i(fooF@unc()) {}\n"
                   "    int fooFunc() {return 2;}\n"
                   "    int m_i;\n"
                   "};\n")
            << QByteArray();

        // Check: No trigger for void functions.
        QTest::newRow("noVoidFunction")
            << QByteArray(
                   "void foo() {}\n"
                   "void bar() {fo@o();}")
            << QByteArray();

        // Check: No trigger for void member functions.
        QTest::newRow("noVoidMemberFunction")
            << QByteArray(
                   "class Foo {public: void fooFunc();}\n"
                   "void bar() {\n"
                   "    Foo *f = new Foo;\n"
                   "    @f->fooFunc();\n"
                   "}")
            << QByteArray();

        // Check: No trigger for void static member functions.
        QTest::newRow("noVoidStaticMemberFunction")
            << QByteArray(
                   "class Foo {public: static void fooFunc();}\n"
                   "void bar() {\n"
                   "    Foo::fo@oFunc();\n"
                   "}")
            << QByteArray();

        // Check: No trigger for functions in expressions.
        QTest::newRow("noFunctionInExpression")
            << QByteArray(
                   "int foo(int a) {return a;}\n"
                   "int bar() {return 1;}"
                   "void baz() {foo(@bar() + bar());}")
            << QByteArray();

        // Check: No trigger for functions in functions. (QTCREATORBUG-9510)
        QTest::newRow("noFunctionInFunction")
            << QByteArray(
                   "int foo(int a, int b) {return a + b;}\n"
                   "int bar(int a) {return a;}\n"
                   "void baz() {\n"
                   "    int a = foo(ba@r(), bar());\n"
                   "}\n")
            << QByteArray();

        // Check: No trigger for functions in return statements (classes).
        QTest::newRow("noReturnClass1")
            << QByteArray(
                   "class Foo {public: static void fooFunc();}\n"
                   "Foo* bar() {\n"
                   "    return new Fo@o;\n"
                   "}")
            << QByteArray();

        // Check: No trigger for functions in return statements (classes). (QTCREATORBUG-9525)
        QTest::newRow("noReturnClass2")
            << QByteArray(
                   "class Foo {public: int fooFunc();}\n"
                   "int bar() {\n"
                   "    return (new Fo@o)->fooFunc();\n"
                   "}")
            << QByteArray();

        // Check: No trigger for functions in return statements (functions).
        QTest::newRow("noReturnFunc1")
            << QByteArray(
                   "class Foo {public: int fooFunc();}\n"
                   "int bar() {\n"
                   "    return Foo::fooFu@nc();\n"
                   "}")
            << QByteArray();

        // Check: No trigger for functions in return statements (functions). (QTCREATORBUG-9525)
        QTest::newRow("noReturnFunc2")
            << QByteArray(
                   "int bar() {\n"
                   "    return list.firs@t().foo;\n"
                   "}\n")
            << QByteArray();

        // Check: No trigger for functions which does not match in signature.
        QTest::newRow("noSignatureMatch")
            << QByteArray(
                   "int someFunc(int);\n"
                   "\n"
                   "void f()\n"
                   "{\n"
                   "    some@Func();\n"
                   "}")
            << QByteArray();
    }

    void test()
    {
        QFETCH(QByteArray, original);
        QFETCH(QByteArray, expected);
        AssignToLocalVariable factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }
};

QObject *AssignToLocalVariable::createTest() { return new AssignToLocalVariableTest; }

#endif // WITH_TESTS
} // namespace

void registerAssignToLocalVariableQuickfix()
{
    CppQuickFixFactory::registerFactory<AssignToLocalVariable>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <assigntolocalvariable.moc>
#endif
