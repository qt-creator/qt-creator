/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <QtTest>
#include <QDebug>
#include <QTextDocument>
#include <QTextCursor>

#include <cplusplus/AST.h>
#include <cplusplus/ASTVisitor.h>
#include <cplusplus/Bind.h>
#include <cplusplus/Control.h>
#include <cplusplus/CoreTypes.h>
#include <cplusplus/DeprecatedGenTemplateInstance.h>
#include <cplusplus/DiagnosticClient.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Names.h>
#include <cplusplus/Names.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Parser.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Symbols.h>

//TESTED_COMPONENT=src/libs/cplusplus

#define NO_PARSER_OR_SEMANTIC_ERROR_MESSAGES

using namespace CPlusPlus;

class tst_Semantic: public QObject
{
    Q_OBJECT

    QSharedPointer<Control> control;

public:
    tst_Semantic()
        : control(new Control)
    { control->setDiagnosticClient(&diag); }

    TranslationUnit *parse(const QByteArray &source,
                           TranslationUnit::ParseMode mode,
                           LanguageFeatures features)
    {
        const StringLiteral *fileId = control->stringLiteral("<stdin>");
        TranslationUnit *unit = new TranslationUnit(control.data(), fileId);
        unit->setSource(source.constData(), source.length());
        unit->setLanguageFeatures(features);
        unit->parse(mode);
        return unit;
    }

    class Document {
        Q_DISABLE_COPY(Document)

    public:
        Document(TranslationUnit *unit)
            : unit(unit)
            , globals(unit->control()->newNamespace(0, 0))
            , errorCount(0)
        { }

        ~Document()
        { }

        void check()
        {
            QVERIFY(unit);
            QVERIFY(unit->ast());
            Bind bind(unit);
            TranslationUnitAST *ast = unit->ast()->asTranslationUnit();
            QVERIFY(ast);
            bind(ast, globals);
        }

        TranslationUnit *unit;
        Namespace *globals;
        unsigned errorCount;
    };

    class Diagnostic: public DiagnosticClient {
    public:
        int errorCount;

        Diagnostic()
            : errorCount(0)
        { }

        virtual void report(int /*level*/,
                            const StringLiteral *fileName,
                            unsigned line, unsigned column,
                            const char *format, va_list ap)
        {
            ++errorCount;

#ifndef NO_PARSER_OR_SEMANTIC_ERROR_MESSAGES
            qDebug() << fileName->chars()<<':'<<line<<':'<<column<<' '<<QString().vsprintf(format, ap);
#else
            Q_UNUSED(fileName);
            Q_UNUSED(line);
            Q_UNUSED(column);
            Q_UNUSED(format);
            Q_UNUSED(ap);
#endif
        }
    };

    Diagnostic diag;


    QSharedPointer<Document> document(const QByteArray &source, bool enableObjc = false, bool qtMocRun = false, bool enableCxx11 = false)
    {
        LanguageFeatures features;
        features.objCEnabled = enableObjc;
        features.qtEnabled = qtMocRun;
        features.qtMocRunEnabled = qtMocRun;
        features.qtKeywordsEnabled = qtMocRun;
        features.cxx11Enabled = enableCxx11;
        diag.errorCount = 0; // reset the error count.
        TranslationUnit *unit = parse(source, TranslationUnit::ParseTranlationUnit, features);
        QSharedPointer<Document> doc(new Document(unit));
        doc->check();
        doc->errorCount = diag.errorCount;
        return doc;
    }

private slots:
    void function_declaration_1();
    void function_declaration_2();
    void function_definition_1();
    void nested_class_1();
    void alias_declaration_1();
    void typedef_1();
    void typedef_2();
    void typedef_3();
    void const_1();
    void const_2();
    void pointer_to_function_1();

    void template_instance_1();

    void expression_under_cursor_1();

    void bracketed_expression_under_cursor_1();
    void bracketed_expression_under_cursor_2();
    void bracketed_expression_under_cursor_3();
    void bracketed_expression_under_cursor_4();

    void objcClass_1();
    void objcSelector_1();
    void objcSelector_2();

    void q_enum_1();

    void lambda_1();
    void lambda_2();

    void diagnostic_error();

    void enum_constantValue1();
    void enum_constantValue2();
    void enum_constantValue3();
    void enum_constantValue4();
    void enum_constantValue5();
};

void tst_Semantic::function_declaration_1()
{
    QSharedPointer<Document> doc = document("void foo();");
    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);

    Declaration *decl = doc->globals->memberAt(0)->asDeclaration();
    QVERIFY(decl);

    FullySpecifiedType declTy = decl->type();
    Function *funTy = declTy->asFunctionType();
    QVERIFY(funTy);
    QVERIFY(funTy->returnType()->isVoidType());
    QCOMPARE(funTy->argumentCount(), 0U);

    QVERIFY(decl->name()->isNameId());
    const Identifier *funId = decl->name()->asNameId()->identifier();
    QVERIFY(funId);

    const QByteArray foo(funId->chars(), funId->size());
    QCOMPARE(foo, QByteArray("foo"));
}

void tst_Semantic::function_declaration_2()
{
    QSharedPointer<Document> doc = document("void foo(const QString &s);");
    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);

    Declaration *decl = doc->globals->memberAt(0)->asDeclaration();
    QVERIFY(decl);

    FullySpecifiedType declTy = decl->type();
    Function *funTy = declTy->asFunctionType();
    QVERIFY(funTy);
    QVERIFY(funTy->returnType()->isVoidType());
    QCOMPARE(funTy->argumentCount(), 1U);

    // check the formal argument.
    Argument *arg = funTy->argumentAt(0)->asArgument();
    QVERIFY(arg);
    QVERIFY(arg->name());
    QVERIFY(! arg->hasInitializer());

    // check the argument's name.
    const Identifier *argNameId = arg->name()->asNameId();
    QVERIFY(argNameId);

    const Identifier *argId = argNameId->identifier();
    QVERIFY(argId);

    QCOMPARE(QByteArray(argId->chars(), argId->size()), QByteArray("s"));

    // check the type of the formal argument
    FullySpecifiedType argTy = arg->type();
    QVERIFY(argTy->isReferenceType());
    QVERIFY(argTy->asReferenceType()->elementType().isConst());
    NamedType *namedTy = argTy->asReferenceType()->elementType()->asNamedType();
    QVERIFY(namedTy);
    QVERIFY(namedTy->name());
    const Identifier *namedTypeId = namedTy->name()->asNameId()->identifier();
    QVERIFY(namedTypeId);
    QCOMPARE(QByteArray(namedTypeId->chars(), namedTypeId->size()),
             QByteArray("QString"));

    QVERIFY(decl->name()->isNameId());
    const Identifier *funId = decl->name()->asNameId()->identifier();
    QVERIFY(funId);

    const QByteArray foo(funId->chars(), funId->size());
    QCOMPARE(foo, QByteArray("foo"));
}

void tst_Semantic::function_definition_1()
{
    QSharedPointer<Document> doc = document("void foo() {}");
    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);

    Function *funTy = doc->globals->memberAt(0)->asFunction();
    QVERIFY(funTy);
    QVERIFY(funTy->returnType()->isVoidType());
    QCOMPARE(funTy->argumentCount(), 0U);

    QVERIFY(funTy->name()->isNameId());
    const Identifier *funId = funTy->name()->asNameId()->identifier();
    QVERIFY(funId);

    const QByteArray foo(funId->chars(), funId->size());
    QCOMPARE(foo, QByteArray("foo"));
}

void tst_Semantic::nested_class_1()
{
    QSharedPointer<Document> doc = document(
"class Object {\n"
"    class Data;\n"
"    Data *d;\n"
"};\n"
"class Object::Data {\n"
"   Object *q;\n"
"};\n"
    );
    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 2U);

    Class *classObject = doc->globals->memberAt(0)->asClass();
    QVERIFY(classObject);
    QVERIFY(classObject->name());
    const Identifier *classObjectNameId = classObject->name()->asNameId();
    QVERIFY(classObjectNameId);
    const Identifier *objectId = classObjectNameId->identifier();
    QCOMPARE(QByteArray(objectId->chars(), objectId->size()), QByteArray("Object"));
    QCOMPARE(classObject->baseClassCount(), 0U);
    QCOMPARE(classObject->memberCount(), 2U);

    Class *classObjectData = doc->globals->memberAt(1)->asClass();
    QVERIFY(classObjectData);
    QVERIFY(classObjectData->name());
    const QualifiedNameId *q = classObjectData->name()->asQualifiedNameId();
    QVERIFY(q);
    QVERIFY(q->base());
    QVERIFY(q->base()->asNameId());
    QCOMPARE(q->base(), classObject->name());
    QVERIFY(q->name());
    QVERIFY(q->name()->asNameId());
    QCOMPARE(doc->globals->find(q->base()->asNameId()->identifier()), classObject);

    Declaration *decl = classObjectData->memberAt(0)->asDeclaration();
    QVERIFY(decl);
    PointerType *ptrTy = decl->type()->asPointerType();
    QVERIFY(ptrTy);
    NamedType *namedTy = ptrTy->elementType()->asNamedType();
    QVERIFY(namedTy);
    QVERIFY(namedTy->name()->asNameId());
    QCOMPARE(namedTy->name()->asNameId()->identifier(), objectId);
}

void tst_Semantic::alias_declaration_1()
{
    QSharedPointer<Document> doc = document(
                "using wobble = int;\n"
                , false, false, true);

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);

    Declaration *decl = doc->globals->memberAt(0)->asDeclaration();
    QVERIFY(decl->name());
    QVERIFY(decl->name()->identifier());
    QCOMPARE(decl->name()->identifier()->chars(), "wobble");

    QVERIFY(decl->isTypedef());
    QVERIFY(decl->type().isTypedef());
    QVERIFY(decl->type()->isIntegerType());
}

void tst_Semantic::typedef_1()
{
    QSharedPointer<Document> doc = document(
"typedef struct {\n"
"   int x, y;\n"
"} Point;\n"
"int main() {\n"
"   Point pt;\n"
"   pt.x = 1;\n"
"}\n"
    );

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 3U);

    Class *anonStruct = doc->globals->memberAt(0)->asClass();
    QVERIFY(anonStruct);
    QCOMPARE(anonStruct->memberCount(), 2U);

    Declaration *typedefPointDecl = doc->globals->memberAt(1)->asDeclaration();
    QVERIFY(typedefPointDecl);
    QVERIFY(typedefPointDecl->isTypedef());
    QCOMPARE(typedefPointDecl->type()->asClassType(), anonStruct);

    Function *mainFun = doc->globals->memberAt(2)->asFunction();
    QVERIFY(mainFun);
}

void tst_Semantic::typedef_2()
{
    QSharedPointer<Document> doc = document(
"struct _Point {\n"
"   int x, y;\n"
"};\n"
"typedef _Point Point;\n"
"int main() {\n"
"   Point pt;\n"
"   pt.x = 1;\n"
"}\n"
    );

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 3U);

    Class *_pointStruct= doc->globals->memberAt(0)->asClass();
    QVERIFY(_pointStruct);
    QCOMPARE(_pointStruct->memberCount(), 2U);

    Declaration *typedefPointDecl = doc->globals->memberAt(1)->asDeclaration();
    QVERIFY(typedefPointDecl);
    QVERIFY(typedefPointDecl->isTypedef());
    QVERIFY(typedefPointDecl->type()->isNamedType());
    QCOMPARE(typedefPointDecl->type()->asNamedType()->name(), _pointStruct->name());

    Function *mainFun = doc->globals->memberAt(2)->asFunction();
    QVERIFY(mainFun);
}

void tst_Semantic::typedef_3()
{
    QSharedPointer<Document> doc = document(
"typedef struct {\n"
"   int x, y;\n"
"} *PointPtr;\n"
    );

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 2U);

    Class *_pointStruct= doc->globals->memberAt(0)->asClass();
    QVERIFY(_pointStruct);
    QCOMPARE(_pointStruct->memberCount(), 2U);

    Declaration *typedefPointDecl = doc->globals->memberAt(1)->asDeclaration();
    QVERIFY(typedefPointDecl);
    QVERIFY(typedefPointDecl->isTypedef());
    QVERIFY(typedefPointDecl->type()->isPointerType());
    QCOMPARE(typedefPointDecl->type()->asPointerType()->elementType()->asClassType(),
             _pointStruct);
}

void tst_Semantic::const_1()
{
    QSharedPointer<Document> doc = document("\n"
"int foo(const int *s);\n"
    );

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);

    Declaration *decl = doc->globals->memberAt(0)->asDeclaration();
    QVERIFY(decl);
    QVERIFY(decl->type()->isFunctionType());
    Function *funTy = decl->type()->asFunctionType();
    QVERIFY(funTy->returnType()->isIntegerType());
    QCOMPARE(funTy->argumentCount(), 1U);
    Argument *arg = funTy->argumentAt(0)->asArgument();
    QVERIFY(arg);
    QVERIFY(! arg->type().isConst());
    QVERIFY(arg->type()->isPointerType());
    QVERIFY(arg->type()->asPointerType()->elementType().isConst());
    QVERIFY(arg->type()->asPointerType()->elementType()->isIntegerType());
}

void tst_Semantic::const_2()
{
    QSharedPointer<Document> doc = document("\n"
"int foo(char * const s);\n"
    );

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);

    Declaration *decl = doc->globals->memberAt(0)->asDeclaration();
    QVERIFY(decl);
    QVERIFY(decl->type()->isFunctionType());
    Function *funTy = decl->type()->asFunctionType();
    QVERIFY(funTy->returnType()->isIntegerType());
    QCOMPARE(funTy->argumentCount(), 1U);
    Argument *arg = funTy->argumentAt(0)->asArgument();
    QVERIFY(arg);
    QVERIFY(arg->type().isConst());
    QVERIFY(arg->type()->isPointerType());
    QVERIFY(! arg->type()->asPointerType()->elementType().isConst());
    QVERIFY(arg->type()->asPointerType()->elementType()->isIntegerType());
}

void tst_Semantic::pointer_to_function_1()
{
    QSharedPointer<Document> doc = document("void (*QtSomething)();");
    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);

    Declaration *decl = doc->globals->memberAt(0)->asDeclaration();
    QVERIFY(decl);

    PointerType *ptrTy = decl->type()->asPointerType();
    QVERIFY(ptrTy);

    Function *funTy = ptrTy->elementType()->asFunctionType();
    QVERIFY(funTy);

    QEXPECT_FAIL("", "Requires initialize enclosing scope of pointer-to-function symbols", Continue);
    QVERIFY(funTy->enclosingScope());

    QEXPECT_FAIL("", "Requires initialize enclosing scope of pointer-to-function symbols", Continue);
    QCOMPARE(funTy->enclosingScope(), decl->enclosingScope());
}

void tst_Semantic::template_instance_1()
{
    QSharedPointer<Document> doc = document("template <typename _Tp> class QList { void append(const _Tp &value); };");
    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);

    Template *templ = doc->globals->memberAt(0)->asTemplate();
    QVERIFY(templ);

    Declaration *decl = templ->memberAt(1)->asClass()->memberAt(0)->asDeclaration();
    QVERIFY(decl);

    FullySpecifiedType templArgs[] = { control->integerType(IntegerType::Int) };
    const Name *templId = control->templateNameId(control->identifier("QList"), false, templArgs, 1);

    FullySpecifiedType genTy = DeprecatedGenTemplateInstance::instantiate(templId, decl, control);

    Overview oo;
    oo.showReturnTypes = true;

    const QString genDecl = oo.prettyType(genTy);
    QCOMPARE(genDecl, QString::fromLatin1("void (const int &)"));
}

void tst_Semantic::expression_under_cursor_1()
{
    const QString plainText = "void *ptr = foo(10, bar";

    QTextDocument textDocument;
    textDocument.setPlainText(plainText);

    QTextCursor tc(&textDocument);
    tc.movePosition(QTextCursor::End);

    ExpressionUnderCursor expressionUnderCursor(LanguageFeatures::defaultFeatures());
    const QString expression = expressionUnderCursor(tc);

    QCOMPARE(expression, QString("bar"));
}

void tst_Semantic::bracketed_expression_under_cursor_1()
{
    const QString plainText = "int i = 0, j[1], k = j[i";

    QTextDocument textDocument;
    textDocument.setPlainText(plainText);

    QTextCursor tc(&textDocument);
    tc.movePosition(QTextCursor::End);

    ExpressionUnderCursor expressionUnderCursor(LanguageFeatures::defaultFeatures());
    const QString expression = expressionUnderCursor(tc);

    QCOMPARE(expression, QString("i"));
}

void tst_Semantic::bracketed_expression_under_cursor_2()
{
    const QString plainText = "[receiver msg";

    QTextDocument textDocument;
    textDocument.setPlainText(plainText);

    QTextCursor tc(&textDocument);
    tc.movePosition(QTextCursor::End);

    ExpressionUnderCursor expressionUnderCursor(LanguageFeatures::defaultFeatures());
    const QString expression = expressionUnderCursor(tc);

    QCOMPARE(expression, plainText);
}

void tst_Semantic::bracketed_expression_under_cursor_3()
{
    const QString plainText = "if ([receiver message";

    QTextDocument textDocument;
    textDocument.setPlainText(plainText);

    QTextCursor tc(&textDocument);
    tc.movePosition(QTextCursor::End);

    ExpressionUnderCursor expressionUnderCursor(LanguageFeatures::defaultFeatures());
    const QString expression = expressionUnderCursor(tc);

    QCOMPARE(expression, QString("[receiver message"));
}

void tst_Semantic::bracketed_expression_under_cursor_4()
{
    const QString plainText = "int i = 0, j[1], k = j[(i == 0) ? 0 : i";

    QTextDocument textDocument;
    textDocument.setPlainText(plainText);

    QTextCursor tc(&textDocument);
    tc.movePosition(QTextCursor::End);

    ExpressionUnderCursor expressionUnderCursor(LanguageFeatures::defaultFeatures());
    const QString expression = expressionUnderCursor(tc);

    QCOMPARE(expression, QString("i"));
}

void tst_Semantic::objcClass_1()
{
    QSharedPointer<Document> doc = document("\n"
                                            "@interface Zoo {} +(id)alloc;-(id)init;@end\n"
                                            "@implementation Zoo\n"
                                            "+(id)alloc{}\n"
                                            "-(id)init{}\n"
                                            "-(void)dealloc{}\n"
                                            "@end\n",
                                            true);

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 2U);

    ObjCClass *iface = doc->globals->memberAt(0)->asObjCClass();
    QVERIFY(iface);
    QVERIFY(iface->isInterface());
    QCOMPARE(iface->memberCount(), 2U);

    ObjCClass *impl = doc->globals->memberAt(1)->asObjCClass();
    QVERIFY(impl);
    QVERIFY(!impl->isInterface());
    QCOMPARE(impl->memberCount(), 3U);

    ObjCMethod *allocMethod = impl->memberAt(0)->asObjCMethod();
    QVERIFY(allocMethod);
    QVERIFY(allocMethod->name() && allocMethod->name()->identifier());
    QCOMPARE(QLatin1String(allocMethod->name()->identifier()->chars()), QLatin1String("alloc"));
    QVERIFY(allocMethod->isStatic());

    ObjCMethod *deallocMethod = impl->memberAt(2)->asObjCMethod();
    QVERIFY(deallocMethod);
    QVERIFY(deallocMethod->name() && deallocMethod->name()->identifier());
    QCOMPARE(QLatin1String(deallocMethod->name()->identifier()->chars()), QLatin1String("dealloc"));
    QVERIFY(!deallocMethod->isStatic());
}

void tst_Semantic::objcSelector_1()
{
    QSharedPointer<Document> doc = document("\n"
                                            "@interface A {}\n"
                                            "-(void) a:(int)a    b:(int)b c:(int)c;\n"
                                            "@end\n",
                                            true);

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);

    ObjCClass *iface = doc->globals->memberAt(0)->asObjCClass();
    QVERIFY(iface);
    QVERIFY(iface->isInterface());
    QCOMPARE(iface->memberCount(), 1U);

    Declaration *decl = iface->memberAt(0)->asDeclaration();
    QVERIFY(decl);
    QVERIFY(decl->name());
    const SelectorNameId *selId = decl->name()->asSelectorNameId();
    QVERIFY(selId);
    QCOMPARE(selId->nameCount(), 3U);
    QCOMPARE(selId->nameAt(0)->identifier()->chars(), "a");
    QCOMPARE(selId->nameAt(1)->identifier()->chars(), "b");
    QCOMPARE(selId->nameAt(2)->identifier()->chars(), "c");
}

class CollectSelectors: public ASTVisitor
{
public:
    CollectSelectors(TranslationUnit *xUnit): ASTVisitor(xUnit) {}

    QList<ObjCSelectorAST *> operator()() {
        selectors.clear();
        accept(translationUnit()->ast());
        return selectors;
    }

    virtual bool visit(ObjCSelectorAST *ast) {selectors.append(ast); return false;}

private:
    QList<ObjCSelectorAST *> selectors;
};

void tst_Semantic::objcSelector_2()
{
    QSharedPointer<Document> doc = document("\n"
                                            "@implementation A {}\n"
                                            "-(SEL)x {\n"
                                            "  return @selector(a:b:c:);\n"
                                            "}\n"
                                            "@end\n",
                                            true);

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);

    ObjCClass *iface = doc->globals->memberAt(0)->asObjCClass();
    QVERIFY(iface);
    QCOMPARE(iface->memberCount(), 1U);

    QList<ObjCSelectorAST*>selectors = CollectSelectors(doc->unit)();
    QCOMPARE(selectors.size(), 2);

    ObjCSelectorAST *sel = selectors.at(1)->asObjCSelector();
    QVERIFY(sel);

    const SelectorNameId *selId = sel->name->asSelectorNameId();
    QVERIFY(selId);
    QCOMPARE(selId->nameCount(), 3U);
    QCOMPARE(selId->nameAt(0)->identifier()->chars(), "a");
    QCOMPARE(selId->nameAt(1)->identifier()->chars(), "b");
    QCOMPARE(selId->nameAt(2)->identifier()->chars(), "c");
}

void tst_Semantic::q_enum_1()
{
    QSharedPointer<Document> doc = document("\n"
                                            "class Tst {\n"
                                            "Q_ENUMS(e)\n"
                                            "public:\n"
                                            "enum e { x, y };\n"
                                            "};\n",
                                            false, true);

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);
    QVERIFY(doc->unit);
    TranslationUnitAST *xUnit = doc->unit->ast()->asTranslationUnit();
    QVERIFY(xUnit);
    SimpleDeclarationAST *tstDecl = xUnit->declaration_list->value->asSimpleDeclaration();
    QVERIFY(tstDecl);
    ClassSpecifierAST *tst = tstDecl->decl_specifier_list->value->asClassSpecifier();
    QVERIFY(tst);
    QtEnumDeclarationAST *qtEnum = tst->member_specifier_list->value->asQtEnumDeclaration();
    QVERIFY(qtEnum);
    SimpleNameAST *e = qtEnum->enumerator_list->value->asSimpleName();
    QVERIFY(e);
    QCOMPARE(doc->unit->spell(e->identifier_token), "e");
    QCOMPARE(e->name->identifier()->chars(), "e");
}

void tst_Semantic::lambda_1()
{
    QSharedPointer<Document> doc = document("\n"
                                            "void f() {\n"
                                            "  auto func = [](int a, int b) {return a + b;};\n"
                                            "}\n", false, false, true);

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);
}

void tst_Semantic::lambda_2()
{
    QSharedPointer<Document> doc = document(
                "\n"
                "class A {\n"
                "  void f(int i = [](){});\n"
                "};\n"
                , false, false, true);

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);
    Class *A = doc->globals->memberAt(0)->asClass();
    QVERIFY(A);
    QCOMPARE(A->memberCount(), 1U);
    Declaration *d = A->memberAt(0)->asDeclaration();
    QCOMPARE(d->name()->identifier()->chars(), "f");
    Function *ty = d->type()->asFunctionType();
    QVERIFY(ty);
    QCOMPARE(ty->argumentCount(), 1U);
    Argument *arg = ty->argumentAt(0)->asArgument();
    QVERIFY(arg);
    const StringLiteral *init = arg->initializer();
    QVERIFY(init);
    QCOMPARE(init->chars(), "[](){}");
}


void tst_Semantic::diagnostic_error()
{
    QSharedPointer<Document> doc = document("\n"
                                            "class Foo {}\n",
                                            false, false);

    QCOMPARE(doc->errorCount, 1U);
    QCOMPARE(doc->globals->memberCount(), 1U);
}

namespace {
void testEnumaratorDeclarator(Enum *e, int enumDeclIndex, const char *expectedConstantValue)
{
    Declaration *enumMemberDeclaration = e->memberAt(enumDeclIndex)->asDeclaration();
    QVERIFY(enumMemberDeclaration);
    EnumeratorDeclaration *enumeratorDeclaration = enumMemberDeclaration->asEnumeratorDeclarator();
    QVERIFY(enumeratorDeclaration);
    if (const StringLiteral *constantValue = enumeratorDeclaration->constantValue())
        QCOMPARE(constantValue->chars(), expectedConstantValue);
    else
        QVERIFY(!expectedConstantValue);
}
} // anonymous

void tst_Semantic::enum_constantValue1()
{
    QSharedPointer<Document> doc = document("\n"
                                            "enum {\n"
                                            "E1,\n"
                                            "E2,\n"
                                            "E3\n"
                                            "};\n"
                                            );

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);
    Enum *e = doc->globals->memberAt(0)->asEnum();
    QVERIFY(e);
    QCOMPARE(e->memberCount(), 3U);

    testEnumaratorDeclarator(e, 0, "0");
    testEnumaratorDeclarator(e, 1, "1");
    testEnumaratorDeclarator(e, 2, "2");
}

void tst_Semantic::enum_constantValue2()
{
    QSharedPointer<Document> doc = document("\n"
                                            "enum {\n"
                                            "E1=10,\n"
                                            "E2,\n"
                                            "E3\n"
                                            "};\n"
                                            );

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);
    Enum *e = doc->globals->memberAt(0)->asEnum();
    QVERIFY(e);
    QCOMPARE(e->memberCount(), 3U);

    testEnumaratorDeclarator(e, 0, "10");
    testEnumaratorDeclarator(e, 1, "11");
    testEnumaratorDeclarator(e, 2, "12");
}

void tst_Semantic::enum_constantValue3()
{
    QSharedPointer<Document> doc = document("\n"
                                            "enum {\n"
                                            "E1,\n"
                                            "E2=10,\n"
                                            "E3\n"
                                            "};\n"
                                            );

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);
    Enum *e = doc->globals->memberAt(0)->asEnum();
    QVERIFY(e);
    QCOMPARE(e->memberCount(), 3U);

    testEnumaratorDeclarator(e, 0, "0");
    testEnumaratorDeclarator(e, 1, "10");
    testEnumaratorDeclarator(e, 2, "11");
}

void tst_Semantic::enum_constantValue4()
{
    QSharedPointer<Document> doc = document("\n"
                                            "enum {\n"
                                            "E1,\n"
                                            "E2=E1+10,\n"
                                            "E3,\n"
                                            "E4=10,\n"
                                            "E5\n"
                                            "};\n"
                                            );

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);
    Enum *e = doc->globals->memberAt(0)->asEnum();
    QVERIFY(e);
    QCOMPARE(e->memberCount(), 5U);

    testEnumaratorDeclarator(e, 0, "0");
    testEnumaratorDeclarator(e, 1, "E1+10");
    testEnumaratorDeclarator(e, 2, NULL);
    testEnumaratorDeclarator(e, 3, "10");
    testEnumaratorDeclarator(e, 4, "11");
}

void tst_Semantic::enum_constantValue5()
{
    QSharedPointer<Document> doc = document("\n"
                                            "enum {\n"
                                            "E1,\n"
                                            "E2=E1,\n"
                                            "E3,\n"
                                            "E4=E3,\n"
                                            "E5\n"
                                            "};\n"
                                            );

    QCOMPARE(doc->errorCount, 0U);
    QCOMPARE(doc->globals->memberCount(), 1U);
    Enum *e = doc->globals->memberAt(0)->asEnum();
    QVERIFY(e);
    QCOMPARE(e->memberCount(), 5U);

    testEnumaratorDeclarator(e, 0, "0");
    testEnumaratorDeclarator(e, 1, "0");
    testEnumaratorDeclarator(e, 2, "1");
    testEnumaratorDeclarator(e, 3, "1");
    testEnumaratorDeclarator(e, 4, "2");
}

QTEST_MAIN(tst_Semantic)
#include "tst_semantic.moc"
