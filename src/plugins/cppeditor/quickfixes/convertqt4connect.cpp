// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "convertqt4connect.h"

#include "../cppcodestylesettings.h"
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
using namespace TextEditor;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

class ConvertQt4ConnectOperation: public CppQuickFixOperation
{
public:
    ConvertQt4ConnectOperation(const CppQuickFixInterface &interface, const ChangeSet &changes)
        : CppQuickFixOperation(interface, 1), m_changes(changes)
    {
        setDescription(Tr::tr("Convert connect() to Qt 5 Style"));
    }

private:
    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());
        currentFile->setChangeSet(m_changes);
        currentFile->apply();
    }

    const ChangeSet m_changes;
};

static Symbol *skipForwardDeclarations(const QList<Symbol *> &symbols)
{
    for (Symbol *symbol : symbols) {
        if (!symbol->type()->asForwardClassDeclarationType())
            return symbol;
    }

    return nullptr;
}

static bool findRawAccessFunction(Class *klass, PointerType *pointerType, QString *objAccessFunction)
{
    QList<Function *> candidates;
    for (auto it = klass->memberBegin(), end = klass->memberEnd(); it != end; ++it) {
        if (Function *func = (*it)->asFunction()) {
            const Name *funcName = func->name();
            if (!funcName->asOperatorNameId()
                && !funcName->asConversionNameId()
                && func->returnType().type() == pointerType
                && func->isConst()
                && func->argumentCount() == 0) {
                candidates << func;
            }
        }
    }
    const Name *funcName = nullptr;
    switch (candidates.size()) {
    case 0:
        return false;
    case 1:
        funcName = candidates.first()->name();
        break;
    default:
        // Multiple candidates - prefer a function named data
        for (Function *func : std::as_const(candidates)) {
            if (!strcmp(func->name()->identifier()->chars(), "data")) {
                funcName = func->name();
                break;
            }
        }
        if (!funcName)
            funcName = candidates.first()->name();
    }
    const Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    *objAccessFunction = QLatin1Char('.') + oo.prettyName(funcName) + QLatin1String("()");
    return true;
}

static PointerType *determineConvertedType(
    NamedType *namedType, const LookupContext &context, Scope *scope, QString *objAccessFunction)
{
    if (!namedType)
        return nullptr;
    if (ClassOrNamespace *binding = context.lookupType(namedType->name(), scope)) {
        if (Symbol *objectClassSymbol = skipForwardDeclarations(binding->symbols())) {
            if (Class *klass = objectClassSymbol->asClass()) {
                for (auto it = klass->memberBegin(), end = klass->memberEnd(); it != end; ++it) {
                    if (Function *func = (*it)->asFunction()) {
                        if (const ConversionNameId *conversionName =
                            func->name()->asConversionNameId()) {
                            if (PointerType *type = conversionName->type()->asPointerType()) {
                                if (findRawAccessFunction(klass, type, objAccessFunction))
                                    return type;
                            }
                        }
                    }
                }
            }
        }
    }

    return nullptr;
}

static Class *senderOrReceiverClass(
    const CppQuickFixInterface &interface,
    const CppRefactoringFilePtr &file,
    const ExpressionAST *objectPointerAST,
    Scope *objectPointerScope,
    QString *objAccessFunction)
{
    const LookupContext &context = interface.context();

    QByteArray objectPointerExpression;
    if (objectPointerAST)
        objectPointerExpression = file->textOf(objectPointerAST).toUtf8();
    else
        objectPointerExpression = "this";

    TypeOfExpression toe;
    toe.setExpandTemplates(true);
    toe.init(interface.semanticInfo().doc, interface.snapshot(), context.bindings());
    const QList<LookupItem> objectPointerExpressions = toe(objectPointerExpression,
                                                           objectPointerScope, TypeOfExpression::Preprocess);
    QTC_ASSERT(!objectPointerExpressions.isEmpty(), return nullptr);

    Type *objectPointerTypeBase = objectPointerExpressions.first().type().type();
    QTC_ASSERT(objectPointerTypeBase, return nullptr);

    PointerType *objectPointerType = objectPointerTypeBase->asPointerType();
    if (!objectPointerType) {
        objectPointerType = determineConvertedType(objectPointerTypeBase->asNamedType(), context,
                                                   objectPointerScope, objAccessFunction);
    }
    QTC_ASSERT(objectPointerType, return nullptr);

    Type *objectTypeBase = objectPointerType->elementType().type(); // Dereference
    QTC_ASSERT(objectTypeBase, return nullptr);

    NamedType *objectType = objectTypeBase->asNamedType();
    QTC_ASSERT(objectType, return nullptr);

    ClassOrNamespace *objectClassCON = context.lookupType(objectType->name(), objectPointerScope);
    if (!objectClassCON) {
        objectClassCON = objectPointerExpressions.first().binding();
        QTC_ASSERT(objectClassCON, return nullptr);
    }
    QTC_ASSERT(!objectClassCON->symbols().isEmpty(), return nullptr);

    Symbol *objectClassSymbol = skipForwardDeclarations(objectClassCON->symbols());
    QTC_ASSERT(objectClassSymbol, return nullptr);

    return objectClassSymbol->asClass();
}

static bool findConnectReplacement(
    const CppQuickFixInterface &interface,
    const ExpressionAST *objectPointerAST,
    const QtMethodAST *methodAST,
    const CppRefactoringFilePtr &file,
    QString *replacement,
    QString *objAccessFunction)
{
    // Get name of method
    if (!methodAST->declarator || !methodAST->declarator->core_declarator)
        return false;

    DeclaratorIdAST *methodDeclIdAST = methodAST->declarator->core_declarator->asDeclaratorId();
    if (!methodDeclIdAST)
        return false;

    NameAST *methodNameAST = methodDeclIdAST->name;
    if (!methodNameAST)
        return false;

    // Lookup object pointer type
    Scope *scope = file->scopeAt(methodAST->firstToken());
    Class *objectClass = senderOrReceiverClass(interface, file, objectPointerAST, scope,
                                               objAccessFunction);
    QTC_ASSERT(objectClass, return false);

    // Look up member function in call, including base class members.
    const LookupContext &context = interface.context();
    const QList<LookupItem> methodResults = context.lookup(methodNameAST->name, objectClass);
    if (methodResults.isEmpty())
        return false; // Maybe mis-spelled signal/slot name

    Scope *baseClassScope = methodResults.at(0).scope(); // FIXME: Handle overloads
    QTC_ASSERT(baseClassScope, return false);

    Class *classOfMethod = baseClassScope->asClass(); // Declaration point of signal/slot
    QTC_ASSERT(classOfMethod, return false);

    Symbol *method = methodResults.at(0).declaration();
    QTC_ASSERT(method, return false);

    // Minimize qualification
    Control *control = context.bindings()->control().get();
    ClassOrNamespace *functionCON = context.lookupParent(scope);
    const Name *shortName = LookupContext::minimalName(method, functionCON, control);
    if (!shortName->asQualifiedNameId())
        shortName = control->qualifiedNameId(classOfMethod->name(), shortName);

    const Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    *replacement = QLatin1Char('&') + oo.prettyName(shortName);
    return true;
}

static bool onConnectOrDisconnectCall(AST *ast, const ExpressionListAST **arguments)
{
    if (!ast)
        return false;

    CallAST *call = ast->asCall();
    if (!call)
        return false;

    if (!call->base_expression)
        return false;

    const IdExpressionAST *idExpr = call->base_expression->asIdExpression();
    if (!idExpr || !idExpr->name || !idExpr->name->name)
        return false;

    const ExpressionListAST *args = call->expression_list;
    if (!arguments)
        return false;

    const Identifier *id = idExpr->name->name->identifier();
    if (!id)
        return false;

    const QByteArray name(id->chars(), id->size());
    if (name != "connect" && name != "disconnect")
        return false;

    if (arguments)
        *arguments = args;
    return true;
}

// Might modify arg* output arguments even if false is returned.
static bool collectConnectArguments(
    const ExpressionListAST *arguments,
    const ExpressionAST **arg1,
    const QtMethodAST **arg2,
    const ExpressionAST **arg3,
    const QtMethodAST **arg4)
{
    if (!arguments || !arg1 || !arg2 || !arg3 || !arg4)
        return false;

    *arg1 = arguments->value;
    arguments = arguments->next;
    if (!arg1 || !arguments)
        return false;

    *arg2 = arguments->value->asQtMethod();
    arguments = arguments->next;
    if (!*arg2 || !arguments)
        return false;

    *arg3 = arguments->value;
    if (!*arg3)
        return false;

    // Take care of three-arg version, with 'this' receiver.
    if (QtMethodAST *receiverMethod = arguments->value->asQtMethod()) {
        *arg3 = nullptr; // Means 'this'
        *arg4 = receiverMethod;
        return true;
    }

    arguments = arguments->next;
    if (!arguments)
        return false;

    *arg4 = arguments->value->asQtMethod();
    if (!*arg4)
        return false;

    return true;
}

//! Converts a Qt 4 QObject::connect() to Qt 5 style.
class ConvertQt4Connect : public CppQuickFixFactory
{
public:
#ifdef WITH_TESTS
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();

        for (int i = path.size(); --i >= 0; ) {
            const ExpressionListAST *arguments;
            if (!onConnectOrDisconnectCall(path.at(i), &arguments))
                continue;

            const ExpressionAST *arg1, *arg3;
            const QtMethodAST *arg2, *arg4;
            if (!collectConnectArguments(arguments, &arg1, &arg2, &arg3, &arg4))
                continue;

            const CppRefactoringFilePtr file = interface.currentFile();

            QString newSignal;
            QString senderAccessFunc;
            if (!findConnectReplacement(interface, arg1, arg2, file, &newSignal, &senderAccessFunc))
                continue;

            QString newMethod;
            QString receiverAccessFunc;
            if (!findConnectReplacement(interface, arg3, arg4, file, &newMethod, &receiverAccessFunc))
                continue;

            ChangeSet changes;
            changes.replace(file->endOf(arg1), file->endOf(arg1), senderAccessFunc);
            changes.replace(file->startOf(arg2), file->endOf(arg2), newSignal);
            if (!arg3)
                newMethod.prepend(QLatin1String("this, "));
            else
                changes.replace(file->endOf(arg3), file->endOf(arg3), receiverAccessFunc);
            changes.replace(file->startOf(arg4), file->endOf(arg4), newMethod);

            result << new ConvertQt4ConnectOperation(interface, changes);
            return;
        }
    }
};

#ifdef WITH_TESTS
using namespace Tests;
class ConvertQt4ConnectTest : public QObject
{
    Q_OBJECT

private slots:
    void testOutOfClass()
    {
        QByteArray prefix =
            "class QObject {};\n"
            "class TestClass : public QObject\n"
            "{\n"
            "public:\n"
            "    void setProp(int) {}\n"
            "    void sigFoo(int) {}\n"
            "};\n"
            "\n"
            "int foo()\n"
            "{\n";

        QByteArray suffix = "\n}\n";

        QByteArray original = prefix
                              + "    TestClass obj;\n"
                                "    conne@ct(&obj, SIGNAL(sigFoo(int)), &obj, SLOT(setProp(int)));"
                              + suffix;

        QByteArray expected = prefix
                              + "    TestClass obj;\n"
                                "    connect(&obj, &TestClass::sigFoo, &obj, &TestClass::setProp);"
                              + suffix;

        QList<TestDocumentPtr> testDocuments;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        ConvertQt4Connect factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testWithinClass_data()
    {
        QTest::addColumn<QByteArray>("original");
        QTest::addColumn<QByteArray>("expected");

        QTest::newRow("four-args-connect")
            << QByteArray("conne@ct(this, SIGNAL(sigFoo(int)), this, SLOT(setProp(int)));")
            << QByteArray("connect(this, &TestClass::sigFoo, this, &TestClass::setProp);");

        QTest::newRow("four-args-disconnect")
            << QByteArray("disconne@ct(this, SIGNAL(sigFoo(int)), this, SLOT(setProp(int)));")
            << QByteArray("disconnect(this, &TestClass::sigFoo, this, &TestClass::setProp);");

        QTest::newRow("three-args-connect")
            << QByteArray("conne@ct(this, SIGNAL(sigFoo(int)), SLOT(setProp(int)));")
            << QByteArray("connect(this, &TestClass::sigFoo, this, &TestClass::setProp);");

        QTest::newRow("template-value")
            << QByteArray("Pointer<TestClass> p;\n"
                          "conne@ct(p.t, SIGNAL(sigFoo(int)), p.t, SLOT(setProp(int)));")
            << QByteArray("Pointer<TestClass> p;\n"
                          "connect(p.t, &TestClass::sigFoo, p.t, &TestClass::setProp);");

        QTest::newRow("implicit-pointer")
            << QByteArray("Pointer<TestClass> p;\n"
                          "conne@ct(p, SIGNAL(sigFoo(int)), p, SLOT(setProp(int)));")
            << QByteArray("Pointer<TestClass> p;\n"
                          "connect(p.data(), &TestClass::sigFoo, p.data(), &TestClass::setProp);");
    }

    void testWithinClass()
    {
        QFETCH(QByteArray, original);
        QFETCH(QByteArray, expected);

        QByteArray prefix =
            "template<class T>\n"
            "struct Pointer\n"
            "{\n"
            "    T *t;\n"
            "    operator T*() const { return t; }\n"
            "    T *data() const { return t; }\n"
            "};\n"
            "class QObject {};\n"
            "class TestClass : public QObject\n"
            "{\n"
            "public:\n"
            "    void setProp(int) {}\n"
            "    void sigFoo(int) {}\n"
            "    void setupSignals();\n"
            "};\n"
            "\n"
            "int TestClass::setupSignals()\n"
            "{\n";

        QByteArray suffix = "\n}\n";

        QList<TestDocumentPtr> testDocuments;
        testDocuments << CppTestDocument::create("file.cpp",
                                                 prefix + original + suffix,
                                                 prefix + expected + suffix);

        ConvertQt4Connect factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testDifferentNamespace()
    {
        const QByteArray prefix =
            "namespace NsA {\n"
            "class ClassA : public QObject\n"
            "{\n"
            "  static ClassA *instance();\n"
            "signals:\n"
            "  void sig();\n"
            "};\n"
            "}\n"
            "\n"
            "namespace NsB {\n"
            "class ClassB : public QObject\n"
            "{\n"
            "  void slot();\n"
            "  void connector() {\n";

        const QByteArray suffix = "  }\n};\n}";

        const QByteArray original = "co@nnect(NsA::ClassA::instance(), SIGNAL(sig()),\n"
                                    "        this, SLOT(slot()));\n";
        const QByteArray expected = "connect(NsA::ClassA::instance(), &NsA::ClassA::sig,\n"
                                    "        this, &ClassB::slot);\n";
        QList<TestDocumentPtr> testDocuments;
        testDocuments << CppTestDocument::create("file.cpp",
                                                 prefix + original + suffix,
                                                 prefix + expected + suffix);

        ConvertQt4Connect factory;
        QuickFixOperationTest(testDocuments, &factory);
    }
};

QObject *ConvertQt4Connect::createTest()
{
    return new ConvertQt4ConnectTest;
}
#endif // WITH_TESTS

} // namespace

void registerConvertQt4ConnectQuickfix()
{
    CppQuickFixFactory::registerFactory<ConvertQt4Connect>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <convertqt4connect.moc>
#endif
