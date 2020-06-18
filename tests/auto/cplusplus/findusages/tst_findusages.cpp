/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "../cplusplus_global.h"

#include <QtTest>
#include <QObject>
#include <QList>

#include <cplusplus/AST.h>
#include <cplusplus/ASTVisitor.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/FindUsages.h>
#include <cplusplus/Literals.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Name.h>
#include <cplusplus/Overview.h>
#include <cplusplus/ResolveExpression.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/TranslationUnit.h>

//TESTED_COMPONENT=src/libs/cplusplus
using namespace CPlusPlus;

class CollectNames: public ASTVisitor
{
public:
    CollectNames(TranslationUnit *xUnit): ASTVisitor(xUnit) {}
    QList<NameAST*> operator()(const char *name) {
        _name = name;
        _exprs.clear();

        accept(translationUnit()->ast());

        return _exprs;
    }

    virtual bool preVisit(AST *ast) {
        if (NameAST *nameAst = ast->asName())
            if (!qstrcmp(_name, nameAst->name->identifier()->chars()))
                _exprs.append(nameAst);
        return true;
    }

private:
    QList<NameAST*> _exprs;
    const char *_name;
};

class tst_FindUsages: public QObject
{
    Q_OBJECT

private:
    void dump(const QList<Usage> &usages) const;

private Q_SLOTS:
    void inlineMethod();
    void lambdaCaptureByValue();
    void lambdaCaptureByReference();
    void shadowedNames_1();
    void shadowedNames_2();
    void staticVariables();

    void functionNameFoundInArguments();
    void memberFunctionFalsePositives_QTCREATORBUG2176();
    void resolveTemplateConstructor();
    void templateConstructorVsCallOperator();

    // Qt keywords
    void qproperty_1();

    // Objective-C
    void objc_args();
//    void objc_methods();
//    void objc_fields();
//    void objc_classes();

    // templates
    void instantiateTemplateWithNestedClass();
    void operatorAsteriskOfNestedClassOfTemplateClass_QTCREATORBUG9006();
    void operatorArrowOfNestedClassOfTemplateClass_QTCREATORBUG9005();
    void templateClassParameters();
    void templateClass_className();
    void templateFunctionParameters();

    void anonymousClass_QTCREATORBUG8963();
    void anonymousClass_QTCREATORBUG11859();
    void using_insideGlobalNamespace();
    void using_insideNamespace();
    void using_insideFunction();
    void templatedFunction_QTCREATORBUG9749();

    void usingInDifferentNamespace_QTCREATORBUG7978();

    void unicodeIdentifier();

    void inAlignas();

    void memberAccessAsTemplate();

    void variadicFunctionTemplate();
    void typeTemplateParameterWithDefault();
    void resolveOrder_for_templateFunction_vs_function();
    void templateArrowOperator_with_defaultType();
    void templateSpecialization_with_IntArgument();
    void templateSpecialization_with_BoolArgument();
    void templatePartialSpecialization();
    void templatePartialSpecialization_2();
    void template_SFINAE_1();
    void variableTemplateInExpression();

    void variadicMacros();
};

void tst_FindUsages::dump(const QList<Usage> &usages) const
{
    QTextStream err(stderr, QIODevice::WriteOnly);
    err << "DEBUG  : " << usages.size() << " usages:" << endl;
    foreach (const Usage &usage, usages) {
        err << "DEBUG  : "
            << usage.path << ":"
            << usage.line << ":"
            << usage.col << ":"
            << usage.len << ":"
            << usage.lineText << endl;
    }
}

void tst_FindUsages::inlineMethod()
{
    const QByteArray src = "\n"
                           "class Tst {\n"
                           "  int method(int arg) {\n"
                           "    return arg;\n"
                           "  }\n"
                           "};\n";
    Document::Ptr doc = Document::create("inlineMethod");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 1);

    Snapshot snapshot;
    snapshot.insert(doc);

    Class *tst = doc->globalSymbolAt(0)->asClass();
    QVERIFY(tst);
    QCOMPARE(tst->memberCount(), 1);
    Function *method = tst->memberAt(0)->asFunction();
    QVERIFY(method);
    QCOMPARE(method->argumentCount(), 1);
    Argument *arg = method->argumentAt(0)->asArgument();
    QVERIFY(arg);
    QCOMPARE(arg->identifier()->chars(), "arg");

    FindUsages findUsages(src, doc, snapshot);
    findUsages(arg);
    QCOMPARE(findUsages.usages().size(), 2);
    QCOMPARE(findUsages.references().size(), 2);
}

void tst_FindUsages::lambdaCaptureByValue()
{
    const QByteArray src = "\n"
                           "void f() {\n"
                           "  int test;\n"
                           "  [test] { ++test; };\n"
                           "}\n";
    Document::Ptr doc = Document::create("lambdaCaptureByValue");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 1);

    Snapshot snapshot;
    snapshot.insert(doc);

    Function *f = doc->globalSymbolAt(0)->asFunction();
    QVERIFY(f);
    QCOMPARE(f->memberCount(), 1);
    Block *b = f->memberAt(0)->asBlock();
    QCOMPARE(b->memberCount(), 2);
    Declaration *d = b->memberAt(0)->asDeclaration();
    QVERIFY(d);
    QCOMPARE(d->name()->identifier()->chars(), "test");

    FindUsages findUsages(src, doc, snapshot);
    findUsages(d);
    QCOMPARE(findUsages.usages().size(), 3);
}

void tst_FindUsages::lambdaCaptureByReference()
{
    const QByteArray src = "\n"
                           "void f() {\n"
                           "  int test;\n"
                           "  [&test] { ++test; };\n"
                           "}\n";
    Document::Ptr doc = Document::create("lambdaCaptureByReference");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 1);

    Snapshot snapshot;
    snapshot.insert(doc);

    Function *f = doc->globalSymbolAt(0)->asFunction();
    QVERIFY(f);
    QCOMPARE(f->memberCount(), 1);
    Block *b = f->memberAt(0)->asBlock();
    QCOMPARE(b->memberCount(), 2);
    Declaration *d = b->memberAt(0)->asDeclaration();
    QVERIFY(d);
    QCOMPARE(d->name()->identifier()->chars(), "test");

    FindUsages findUsages(src, doc, snapshot);
    findUsages(d);
    QCOMPARE(findUsages.usages().size(), 3);
}

void tst_FindUsages::shadowedNames_1()
{
    const QByteArray src = "\n"
                           "int a();\n"
                           "struct X{ int a(); };\n"
                           "int X::a() {}\n"
                           "void f(X x) { x.a(); }\n"
                           "void g() { a(); }\n"
                           ;

    Document::Ptr doc = Document::create("shadowedNames_1");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 5);

    Snapshot snapshot;
    snapshot.insert(doc);

    Declaration *d = doc->globalSymbolAt(0)->asDeclaration();
    QVERIFY(d);
    QCOMPARE(d->name()->identifier()->chars(), "a");

    FindUsages findUsages(src, doc, snapshot);
    findUsages(d);
    QCOMPARE(findUsages.usages().size(), 2);
}

void tst_FindUsages::shadowedNames_2()
{
    const QByteArray src = "\n"
                           "int a();\n"
                           "struct X{ int a(); };\n"
                           "int X::a() {}\n"
                           "void f(X x) { x.a(); }\n"
                           "void g() { a(); }\n";

    Document::Ptr doc = Document::create("shadowedNames_2");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 5);

    Snapshot snapshot;
    snapshot.insert(doc);

    Class *c = doc->globalSymbolAt(1)->asClass();
    QVERIFY(c);
    QCOMPARE(c->name()->identifier()->chars(), "X");
    QCOMPARE(c->memberCount(), 1);
    Declaration *d = c->memberAt(0)->asDeclaration();
    QVERIFY(d);
    QCOMPARE(d->name()->identifier()->chars(), "a");

    FindUsages findUsages(src, doc, snapshot);
    findUsages(d);
    QCOMPARE(findUsages.usages().size(), 3);
}

void tst_FindUsages::staticVariables()
{
    const QByteArray src = "\n"
            "struct Outer\n"
            "{\n"
            "    static int Foo;\n"
            "    struct Inner\n"
            "    {\n"
            "        Outer *outer;\n"
            "        void foo();\n"
            "    };\n"
            "};\n"
            "\n"
            "int Outer::Foo = 42;\n"
            "\n"
            "void Outer::Inner::foo()\n"
            "{\n"
            "    Foo  = 7;\n"
            "    Outer::Foo = 7;\n"
            "    outer->Foo = 7;\n"
            "}\n"
            ;
    Document::Ptr doc = Document::create("staticVariables");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 3);

    Snapshot snapshot;
    snapshot.insert(doc);

    Class *c = doc->globalSymbolAt(0)->asClass();
    QVERIFY(c);
    QCOMPARE(c->name()->identifier()->chars(), "Outer");
    QCOMPARE(c->memberCount(), 2);
    Declaration *d = c->memberAt(0)->asDeclaration();
    QVERIFY(d);
    QCOMPARE(d->name()->identifier()->chars(), "Foo");

    FindUsages findUsages(src, doc, snapshot);
    findUsages(d);
    QCOMPARE(findUsages.usages().size(), 5);
}

void tst_FindUsages::functionNameFoundInArguments()
{
    const QByteArray src =
        R"(
void bar();                 // call find usages for bar from here. This is 1st result
void foo(int bar);          // should not be found
void foo(int bar){}         // should not be found
void foo2(int b=bar());     // 2nd result
void foo2(int b=bar()){}    // 3rd result
)";

    Document::Ptr doc = Document::create("functionNameFoundInArguments");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QVERIFY(doc->globalSymbolCount() >= 1);

    Symbol *s = doc->globalSymbolAt(0);
    QCOMPARE(s->name()->identifier()->chars(), "bar");

    Snapshot snapshot;
    snapshot.insert(doc);

    FindUsages find(src, doc, snapshot);
    find(s);

    QCOMPARE(find.usages().size(), 3);

    QCOMPARE(find.usages()[0].line, 1);
    QCOMPARE(find.usages()[0].col, 5);

    QCOMPARE(find.usages()[1].line, 4);
    QCOMPARE(find.usages()[1].col, 16);

    QCOMPARE(find.usages()[2].line, 5);
    QCOMPARE(find.usages()[2].col, 16);
}

void tst_FindUsages::memberFunctionFalsePositives_QTCREATORBUG2176()
{
    const QByteArray src =
        R"(
void otherFunction(int value){}
struct Struct{
    static int foo(){return 1;}
    int bar(){
        int foo=Struct::foo();
        otherFunction(foo);
    }
};
)";

    Document::Ptr doc = Document::create("memberFunctionFalsePositives");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 2);

    Class *s = doc->globalSymbolAt(1)->asClass();
    QVERIFY(s);
    QCOMPARE(s->name()->identifier()->chars(), "Struct");
    QCOMPARE(s->memberCount(), 2);

    Symbol *memberFunctionFoo = s->memberAt(0);
    QVERIFY(memberFunctionFoo);
    QCOMPARE(memberFunctionFoo->name()->identifier()->chars(), "foo");
    QVERIFY(memberFunctionFoo->asFunction());

    Function* bar = s->memberAt(1)->asFunction();
    QVERIFY(bar);
    QCOMPARE(bar->name()->identifier()->chars(), "bar");
    QCOMPARE(bar->memberCount(), 1);

    Block* block = bar->memberAt(0)->asBlock();
    QVERIFY(block);
    QCOMPARE(block->memberCount(), 1);

    Symbol *variableFoo = block->memberAt(0);
    QVERIFY(variableFoo);
    QCOMPARE(variableFoo->name()->identifier()->chars(), "foo");
    QVERIFY(variableFoo->asDeclaration());

    Snapshot snapshot;
    snapshot.insert(doc);

    FindUsages find(src, doc, snapshot);

    find(memberFunctionFoo);
    QCOMPARE(find.usages().size(), 2);

    QCOMPARE(find.usages()[0].line, 3);
    QCOMPARE(find.usages()[0].col, 15);

    QCOMPARE(find.usages()[1].line, 5);
    QCOMPARE(find.usages()[1].col, 24);

    find(variableFoo);
    QCOMPARE(find.usages().size(), 2);

    QCOMPARE(find.usages()[0].line, 5);
    QCOMPARE(find.usages()[0].col, 12);

    QCOMPARE(find.usages()[1].line, 6);
    QCOMPARE(find.usages()[1].col, 22);
}

void tst_FindUsages::resolveTemplateConstructor()
{
    const QByteArray src =
        R"(
struct MyStruct { int value; };
template <class T> struct Tmp {
    T str;
};
template <class T> struct Tmp2 {
    Tmp2(){}
    T str;
};
template <class T> struct Tmp3 {
    Tmp3(int i){}
    T str;
};
int main() {
    auto tmp = Tmp<MyStruct>();
    auto tmp2 = Tmp2<MyStruct>();
    auto tmp3 = Tmp3<MyStruct>(1);
    tmp.str.value;
    tmp2.str.value;
    tmp3.str.value;
    Tmp<MyStruct>().str.value;
    Tmp2<MyStruct>().str.value;
    Tmp3<MyStruct>(1).str.value;
}
)";

    Document::Ptr doc = Document::create("resolveTemplateConstructor");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QVERIFY(doc->globalSymbolCount() == 5);

    Class *s = doc->globalSymbolAt(0)->asClass();
    QVERIFY(s);
    QCOMPARE(s->name()->identifier()->chars(), "MyStruct");
    QCOMPARE(s->memberCount(), 1);

    Declaration *sv = s->memberAt(0)->asDeclaration();
    QVERIFY(sv);
    QCOMPARE(sv->name()->identifier()->chars(), "value");

    Snapshot snapshot;
    snapshot.insert(doc);

    FindUsages find(src, doc, snapshot);
    find(sv);
    QCOMPARE(find.usages().size(), 7);
}

void tst_FindUsages::templateConstructorVsCallOperator()
{
    const QByteArray src =
        R"(
struct MyStruct { int value; };
template<class T> struct Tmp {
    T str;
    MyStruct operator()() { return MyStruct(); }
};
struct Simple {
    MyStruct str;
    MyStruct operator()() { return MyStruct(); }
};
int main()
{
    Tmp<MyStruct>().str.value;
    Tmp<MyStruct>()().value;
    Tmp<MyStruct> t;
    t().value;

    Simple().str.value;
    Simple()().value;
    Simple s;
    s().value;
}
)";

    Document::Ptr doc = Document::create("resolveTemplateConstructor");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QVERIFY(doc->globalSymbolCount() == 4);

    Class *s = doc->globalSymbolAt(0)->asClass();
    QVERIFY(s);
    QCOMPARE(s->name()->identifier()->chars(), "MyStruct");
    QCOMPARE(s->memberCount(), 1);

    Declaration *sv = s->memberAt(0)->asDeclaration();
    QVERIFY(sv);
    QCOMPARE(sv->name()->identifier()->chars(), "value");

    Snapshot snapshot;
    snapshot.insert(doc);

    FindUsages find(src, doc, snapshot);
    find(sv);
    QCOMPARE(find.usages().size(), 7);
}

#if 0
@interface Clazz {} +(void)method:(int)arg; @end
@implementation Clazz +(void)method:(int)arg {
  [Clazz method:arg];
}
@end
#endif
const QByteArray objcSource = "\n"
                              "@interface Clazz {} +(void)method:(int)arg; @end\n"
                              "@implementation Clazz +(void)method:(int)arg {\n"
                              "  [Clazz method:arg];\n"
                              "}\n"
                              "@end\n";

void tst_FindUsages::objc_args()
{
    Document::Ptr doc = Document::create("objc_args");
    doc->setUtf8Source(objcSource);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 2);

    Snapshot snapshot;
    snapshot.insert(doc);

    TranslationUnit *xUnit = doc->translationUnit();
    QList<NameAST*>exprs = CollectNames(xUnit)("arg");
    QCOMPARE(exprs.size(), 3);

    ObjCClass *iface = doc->globalSymbolAt(0)->asObjCClass();
    QVERIFY(iface);
    QVERIFY(iface->isInterface());
    QCOMPARE(iface->memberCount(), 1);

    Declaration *methodIface = iface->memberAt(0)->asDeclaration();
    QVERIFY(methodIface);
    QCOMPARE(methodIface->identifier()->chars(), "method");
    QVERIFY(methodIface->type()->isObjCMethodType());

    ObjCClass *impl = doc->globalSymbolAt(1)->asObjCClass();
    QVERIFY(impl);
    QVERIFY(!impl->isInterface());
    QCOMPARE(impl->memberCount(), 1);

    ObjCMethod *methodImpl = impl->memberAt(0)->asObjCMethod();
    QVERIFY(methodImpl);
    QCOMPARE(methodImpl->identifier()->chars(), "method");
    QCOMPARE(methodImpl->argumentCount(), 1);
    Argument *arg = methodImpl->argumentAt(0)->asArgument();
    QCOMPARE(arg->identifier()->chars(), "arg");

    FindUsages findUsages(objcSource, doc, snapshot);
    findUsages(arg);
    QCOMPARE(findUsages.usages().size(), 2);
    QCOMPARE(findUsages.references().size(), 2);
}

void tst_FindUsages::qproperty_1()
{
    const QByteArray src = "\n"
                           "class Tst: public QObject {\n"
                           "  Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)\n"
                           "public:\n"
                           "  int x() { return _x; }\n"
                           "  void setX(int x) { if (_x != x) { _x = x; emit xChanged(x); } }\n"
                           "signals:\n"
                           "  void xChanged(int);\n"
                           "private:\n"
                           "  int _x;\n"
                           "};\n";
    Document::Ptr doc = Document::create("qproperty_1");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 1);

    Snapshot snapshot;
    snapshot.insert(doc);

    Class *tst = doc->globalSymbolAt(0)->asClass();
    QVERIFY(tst);
    QCOMPARE(tst->memberCount(), 5);
    Function *setX_method = tst->memberAt(2)->asFunction();
    QVERIFY(setX_method);
    QCOMPARE(setX_method->identifier()->chars(), "setX");
    QCOMPARE(setX_method->argumentCount(), 1);

    FindUsages findUsages(src, doc, snapshot);
    findUsages(setX_method);
    QCOMPARE(findUsages.usages().size(), 2);
    QCOMPARE(findUsages.references().size(), 2);
}

void tst_FindUsages::instantiateTemplateWithNestedClass()
{
    const QByteArray src = "\n"
            "struct Foo\n"
            "{ int bar; };\n"
            "template <typename T>\n"
            "struct Template\n"
            "{\n"
            "   struct Nested\n"
            "   {\n"
            "       T t;\n"
            "   }nested;\n"
            "};\n"
            "void f()\n"
            "{\n"
            "   Template<Foo> templateFoo;\n"
            "   templateFoo.nested.t.bar;\n"
            "}\n"
            ;

    Document::Ptr doc = Document::create("simpleTemplate");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 3);

    Snapshot snapshot;
    snapshot.insert(doc);

    Class *classFoo = doc->globalSymbolAt(0)->asClass();
    QVERIFY(classFoo);
    QCOMPARE(classFoo->memberCount(), 1);
    Declaration *barDeclaration = classFoo->memberAt(0)->asDeclaration();
    QVERIFY(barDeclaration);
    QCOMPARE(barDeclaration->name()->identifier()->chars(), "bar");

    FindUsages findUsages(src, doc, snapshot);
    findUsages(barDeclaration);
    QCOMPARE(findUsages.usages().size(), 2);
}

void tst_FindUsages::operatorAsteriskOfNestedClassOfTemplateClass_QTCREATORBUG9006()
{
    const QByteArray src = "\n"
            "struct Foo { int foo; };\n"
            "\n"
            "template<class T>\n"
            "struct Outer\n"
            "{\n"
            "  struct Nested\n"
            "  {\n"
            "    const T &operator*() { return t; }\n"
            "    T t;\n"
            "  };\n"
            "};\n"
            "\n"
            "void bug()\n"
            "{\n"
            "  Outer<Foo>::Nested nested;\n"
            "  (*nested).foo;\n"
            "}\n"
            ;

    Document::Ptr doc = Document::create("operatorAsteriskOfNestedClassOfTemplateClass_QTCREATORBUG9006");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 3);
    Snapshot snapshot;
    snapshot.insert(doc);

    Class *classFoo = doc->globalSymbolAt(0)->asClass();
    QVERIFY(classFoo);
    QCOMPARE(classFoo->memberCount(), 1);
    Declaration *fooDeclaration = classFoo->memberAt(0)->asDeclaration();
    QVERIFY(fooDeclaration);
    QCOMPARE(fooDeclaration->name()->identifier()->chars(), "foo");

    FindUsages findUsages(src, doc, snapshot);
    findUsages(fooDeclaration);

    QCOMPARE(findUsages.usages().size(), 2);
}

void tst_FindUsages::anonymousClass_QTCREATORBUG8963()
{
    const QByteArray src =
            "typedef enum {\n"
            " FIRST\n"
            "} isNotInt;\n"
            "typedef struct {\n"
            " int isint;\n"
            " int isNotInt;\n"
            "} Struct;\n"
            "void foo()\n"
            "{\n"
            " Struct s;\n"
            " s.isint;\n"
            " s.isNotInt;\n"
            " FIRST;\n"
            "}\n"
            ;

    Document::Ptr doc = Document::create("anonymousClass_QTCREATORBUG8963");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 5);

    Snapshot snapshot;
    snapshot.insert(doc);

    Class *structSymbol = doc->globalSymbolAt(2)->asClass();
    QVERIFY(structSymbol);
    QCOMPARE(structSymbol->memberCount(), 2);
    Declaration *isNotIntDeclaration = structSymbol->memberAt(1)->asDeclaration();
    QVERIFY(isNotIntDeclaration);
    QCOMPARE(isNotIntDeclaration->name()->identifier()->chars(), "isNotInt");

    FindUsages findUsages(src, doc, snapshot);
    findUsages(isNotIntDeclaration);

    QCOMPARE(findUsages.usages().size(), 2);
}

void tst_FindUsages::anonymousClass_QTCREATORBUG11859()
{
    const QByteArray src =
            "struct Foo {\n"
            "};\n"
            "typedef struct {\n"
            " int Foo;\n"
            "} Struct;\n"
            "void foo()\n"
            "{\n"
            " Struct s;\n"
            " s.Foo;\n"
            "}\n"
            ;

    Document::Ptr doc = Document::create("anonymousClass_QTCREATORBUG11859");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 4);

    Snapshot snapshot;
    snapshot.insert(doc);

    Class *fooAsStruct = doc->globalSymbolAt(0)->asClass();
    QVERIFY(fooAsStruct);
    Class *structSymbol = doc->globalSymbolAt(1)->asClass();
    QVERIFY(structSymbol);
    QCOMPARE(structSymbol->memberCount(), 1);
    Declaration *fooAsMemberOfAnonymousStruct = structSymbol->memberAt(0)->asDeclaration();
    QVERIFY(fooAsMemberOfAnonymousStruct);
    QCOMPARE(fooAsMemberOfAnonymousStruct->name()->identifier()->chars(), "Foo");

    FindUsages findUsages(src, doc, snapshot);
    findUsages(fooAsStruct);
    QCOMPARE(findUsages.references().size(), 1);

    findUsages(fooAsMemberOfAnonymousStruct);
    QCOMPARE(findUsages.references().size(), 2);
}

void tst_FindUsages::using_insideGlobalNamespace()
{
    const QByteArray src =
            "namespace NS\n"
            "{\n"
            "struct Struct\n"
            "{\n"
            "    int bar;\n"
            "};\n"
            "}\n"
            "using NS::Struct;\n"
            "void foo()\n"
            "{\n"
            "    Struct s;\n"
            "}\n"
            ;

    Document::Ptr doc = Document::create("using_insideGlobalNamespace");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 3);

    Snapshot snapshot;
    snapshot.insert(doc);

    Namespace *nsSymbol = doc->globalSymbolAt(0)->asNamespace();
    QVERIFY(nsSymbol);
    QCOMPARE(nsSymbol->memberCount(), 1);
    Class *structSymbol = nsSymbol->memberAt(0)->asClass();
    QVERIFY(structSymbol);

    FindUsages findUsages(src, doc, snapshot);
    findUsages(structSymbol);

    QCOMPARE(findUsages.usages().size(), 3);
}

void tst_FindUsages::using_insideNamespace()
{
    const QByteArray src =
            "namespace NS\n"
            "{\n"
            "struct Struct\n"
            "{\n"
            "    int bar;\n"
            "};\n"
            "}\n"
            "namespace NS1\n"
            "{\n"
            "using NS::Struct;\n"
            "void foo()\n"
            "{\n"
            "    Struct s;\n"
            "}\n"
            "}\n"
            ;

    Document::Ptr doc = Document::create("using_insideNamespace");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 2);

    Snapshot snapshot;
    snapshot.insert(doc);

    Namespace *nsSymbol = doc->globalSymbolAt(0)->asNamespace();
    QVERIFY(nsSymbol);
    QCOMPARE(nsSymbol->memberCount(), 1);
    Class *structSymbol = nsSymbol->memberAt(0)->asClass();
    QVERIFY(structSymbol);

    FindUsages findUsages(src, doc, snapshot);
    findUsages(structSymbol);

    QCOMPARE(findUsages.usages().size(), 3);
}

void tst_FindUsages::using_insideFunction()
{
    const QByteArray src =
            "namespace NS\n"
            "{\n"
            "struct Struct\n"
            "{\n"
            "    int bar;\n"
            "};\n"
            "}\n"
            "void foo()\n"
            "{\n"
            "    using NS::Struct;\n"
            "    Struct s;\n"
            "}\n"
            ;

    Document::Ptr doc = Document::create("using_insideFunction");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 2);

    Snapshot snapshot;
    snapshot.insert(doc);

    Namespace *nsSymbol = doc->globalSymbolAt(0)->asNamespace();
    QVERIFY(nsSymbol);
    QCOMPARE(nsSymbol->memberCount(), 1);
    Class *structSymbol = nsSymbol->memberAt(0)->asClass();
    QVERIFY(structSymbol);

    FindUsages findUsages(src, doc, snapshot);
    findUsages(structSymbol);

    QCOMPARE(findUsages.usages().size(), 3);
}

void tst_FindUsages::operatorArrowOfNestedClassOfTemplateClass_QTCREATORBUG9005()
{
    const QByteArray src = "\n"
            "struct Foo { int foo; };\n"
            "\n"
            "template<class T>\n"
            "struct Outer\n"
            "{\n"
            "  struct Nested\n"
            "  {\n"
            "    T *operator->() { return 0; }\n"
            "  };\n"
            "};\n"
            "\n"
            "void bug()\n"
            "{\n"
            "  Outer<Foo>::Nested nested;\n"
            "  nested->foo;\n"
            "}\n"
            ;

    Document::Ptr doc = Document::create("operatorArrowOfNestedClassOfTemplateClass_QTCREATORBUG9005");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 3);

    Snapshot snapshot;
    snapshot.insert(doc);

    Class *classFoo = doc->globalSymbolAt(0)->asClass();
    QVERIFY(classFoo);
    QCOMPARE(classFoo->memberCount(), 1);
    Declaration *fooDeclaration = classFoo->memberAt(0)->asDeclaration();
    QVERIFY(fooDeclaration);
    QCOMPARE(fooDeclaration->name()->identifier()->chars(), "foo");

    FindUsages findUsages(src, doc, snapshot);
    findUsages(fooDeclaration);
    QCOMPARE(findUsages.usages().size(), 2);
}

void tst_FindUsages::templateClassParameters()
{
    const QByteArray src = "\n"
            "template <class T>\n"
            "struct TS\n"
            "{\n"
            "    void set(T t) { T t1 = t; }\n"
            "    T get();\n"
            "    T t;\n"
            "};\n"
            ;

    Document::Ptr doc = Document::create("templateClassParameters");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 1);

    Snapshot snapshot;
    snapshot.insert(doc);

    Template *templateClassTS = doc->globalSymbolAt(0)->asTemplate();
    QVERIFY(templateClassTS);
    QCOMPARE(templateClassTS->memberCount(), 2);
    QCOMPARE(templateClassTS->templateParameterCount(), 1);
    TypenameArgument *templArgument = templateClassTS->templateParameterAt(0)->asTypenameArgument();
    QVERIFY(templArgument);

    FindUsages findUsages(src, doc, snapshot);
    findUsages(templArgument);
    QCOMPARE(findUsages.usages().size(), 5);
}

void tst_FindUsages::templateClass_className()
{
    const QByteArray src = "\n"
            "template <class T>\n"
            "struct TS\n"
            "{\n"
            "    TS();\n"
            "    ~TS();\n"
            "};\n"
            "template <class T>\n"
            "TS<T>::TS()\n"
            "{\n"
            "}\n"
            "template <class T>\n"
            "TS<T>::~TS()\n"
            "{\n"
            "}\n"
            ;

    Document::Ptr doc = Document::create("templateClass_className");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 3);

    Snapshot snapshot;
    snapshot.insert(doc);

    Template *templateClassTS = doc->globalSymbolAt(0)->asTemplate();
    QVERIFY(templateClassTS);
    Class *classTS = templateClassTS->memberAt(1)->asClass();
    QVERIFY(classTS);
    QCOMPARE(classTS->memberCount(), 2);

    FindUsages findUsages(src, doc, snapshot);
    findUsages(classTS);
    QCOMPARE(findUsages.usages().size(), 7);
}

void tst_FindUsages::templateFunctionParameters()
{
    const QByteArray src = "\n"
            "template<class T>\n"
            "T foo(T t)\n"
            "{\n"
            "    typename T;\n"
            "}\n"
            ;

    Document::Ptr doc = Document::create("templateFunctionParameters");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 1);

    Snapshot snapshot;
    snapshot.insert(doc);

    Template *templateFunctionTS = doc->globalSymbolAt(0)->asTemplate();
    QVERIFY(templateFunctionTS);
    QCOMPARE(templateFunctionTS->memberCount(), 2);
    QCOMPARE(templateFunctionTS->templateParameterCount(), 1);
    TypenameArgument *templArgument = templateFunctionTS->templateParameterAt(0)->asTypenameArgument();
    QVERIFY(templArgument);

    FindUsages findUsages(src, doc, snapshot);
    findUsages(templArgument);
    QCOMPARE(findUsages.usages().size(), 4);
}

void tst_FindUsages::templatedFunction_QTCREATORBUG9749()
{
    const QByteArray src = "\n"
            "template <class IntType> char *reformatInteger(IntType value, int format) {}\n"
            "void func(int code, int format) {\n"
            "  reformatInteger(code, format);"
            "}\n"
            ;

    Document::Ptr doc = Document::create("templatedFunction_QTCREATORBUG9749");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 2);

    Snapshot snapshot;
    snapshot.insert(doc);

    Template *funcTempl = doc->globalSymbolAt(0)->asTemplate();
    QVERIFY(funcTempl);
    QCOMPARE(funcTempl->memberCount(), 2);
    Function *func = funcTempl->memberAt(1)->asFunction();

    FindUsages findUsages(src, doc, snapshot);
    findUsages(func);
    QCOMPARE(findUsages.usages().size(), 2);
}

void tst_FindUsages::usingInDifferentNamespace_QTCREATORBUG7978()
{
    const QByteArray src = "\n"
            "struct S {};\n"
            "namespace std\n"
            "{\n"
            "    template <typename T> struct shared_ptr{};\n"
            "}\n"
            "namespace NS\n"
            "{\n"
            "    using std::shared_ptr;\n"
            "}\n"
            "void fun()\n"
            "{\n"
            "    NS::shared_ptr<S> p;\n"
            "}\n"
            ;

    Document::Ptr doc = Document::create("usingInDifferentNamespace_QTCREATORBUG7978");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 4);

    Snapshot snapshot;
    snapshot.insert(doc);

    Namespace *ns = doc->globalSymbolAt(1)->asNamespace();
    QVERIFY(ns);
    QCOMPARE(ns->memberCount(), 1);
    Template *templateClass = ns->memberAt(0)->asTemplate();

    FindUsages findUsages(src, doc, snapshot);
    findUsages(templateClass);
    QCOMPARE(findUsages.usages().size(), 3);
}

void tst_FindUsages::unicodeIdentifier()
{
    const QByteArray src = "\n"
            "int var" TEST_UNICODE_IDENTIFIER ";\n"
            "void f() { var" TEST_UNICODE_IDENTIFIER " = 1; }\n";
            ;

    Document::Ptr doc = Document::create("u");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 2);

    Snapshot snapshot;
    snapshot.insert(doc);

    Declaration *declaration = doc->globalSymbolAt(0)->asDeclaration();
    QVERIFY(declaration);

    FindUsages findUsages(src, doc, snapshot);
    findUsages(declaration);
    const QList<Usage> usages = findUsages.usages();
    QCOMPARE(usages.size(), 2);
    QCOMPARE(usages.at(0).len, 7);
    QCOMPARE(usages.at(1).len, 7);
}

void tst_FindUsages::inAlignas()
{
    const QByteArray src = "\n"
            "struct One {};\n"
            "struct alignas(One) Two {};\n"
            ;

    Document::Ptr doc = Document::create("inAlignas");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 2);

    Snapshot snapshot;
    snapshot.insert(doc);

    Class *c = doc->globalSymbolAt(0)->asClass();
    QVERIFY(c);
    QCOMPARE(c->name()->identifier()->chars(), "One");

    FindUsages find(src, doc, snapshot);
    find(c);
    QCOMPARE(find.usages().size(), 2);
    QCOMPARE(find.usages()[0].line, 1);
    QCOMPARE(find.usages()[0].col, 7);
    QCOMPARE(find.usages()[1].line, 2);
    QCOMPARE(find.usages()[1].col, 15);
}

void tst_FindUsages::memberAccessAsTemplate()
{
    const QByteArray src = "\n"
            "struct Foo {};\n"
            "struct Bar {\n"
            "    template <typename T>\n"
            "    T *templateFunc() { return 0; }\n"
            "};\n"
            "struct Test {\n"
            "    Bar member;\n"
            "    void testFunc();\n"
            "};\n"
            "void Test::testFunc() {\n"
            "    member.templateFunc<Foo>();\n"
            "}\n";

    Document::Ptr doc = Document::create("memberAccessAsTemplate");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 4);

    Snapshot snapshot;
    snapshot.insert(doc);

    {   // Test "Foo"
        Class *c = doc->globalSymbolAt(0)->asClass();
        QVERIFY(c);
        QCOMPARE(c->name()->identifier()->chars(), "Foo");

        FindUsages find(src, doc, snapshot);
        find(c);
        QCOMPARE(find.usages().size(), 2);
        QCOMPARE(find.usages()[0].line, 1);
        QCOMPARE(find.usages()[0].col, 7);
        QCOMPARE(find.usages()[1].line, 11);
        QCOMPARE(find.usages()[1].col, 24);
    }

    {   // Test "templateFunc"
        Class *c = doc->globalSymbolAt(1)->asClass();
        QVERIFY(c);
        QCOMPARE(c->name()->identifier()->chars(), "Bar");
        QCOMPARE(c->memberCount(), 1);

        Template *f = c->memberAt(0)->asTemplate();
        QVERIFY(f);
        QCOMPARE(f->name()->identifier()->chars(), "templateFunc");

        FindUsages find(src, doc, snapshot);
        find(f);
        QCOMPARE(find.usages().size(), 2);
        QCOMPARE(find.usages()[0].line, 4);
        QCOMPARE(find.usages()[0].col, 7);
        QCOMPARE(find.usages()[1].line, 11);
        QCOMPARE(find.usages()[1].col, 11);
    }
}

void tst_FindUsages::variadicFunctionTemplate()
{
    const QByteArray src = "struct S{int value;};\n"
                           "template<class ... Types> S foo(Types & ... args){return S();}\n"
                           "int main(){\n"
                           "    foo().value;\n"
                           "    foo(1).value;\n"
                           "    foo(1,2).value;\n"
                           "}";

    Document::Ptr doc = Document::create("variadicFunctionTemplate");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QVERIFY(doc->globalSymbolCount()>=1);

    Snapshot snapshot;
    snapshot.insert(doc);

    {   // Test "S::value"
        Class *c = doc->globalSymbolAt(0)->asClass();
        QVERIFY(c);
        QCOMPARE(c->name()->identifier()->chars(), "S");
        QCOMPARE(c->memberCount(), 1);

        Declaration *v = c->memberAt(0)->asDeclaration();
        QVERIFY(v);
        QCOMPARE(v->name()->identifier()->chars(), "value");

        FindUsages find(src, doc, snapshot);
        find(v);
        QCOMPARE(find.usages().size(), 4);
    }
}

void tst_FindUsages::typeTemplateParameterWithDefault()
{
    const QByteArray src = "struct X{int value;};\n"
                           "struct S{int value;};\n"
                           "template<class T = S> T foo(){return T();}\n"
                           "int main(){\n"
                           "    foo<X>().value;\n"
                           "    foo<S>().value;\n"
                           "    foo().value;\n"     // this is S.value
                           "}";

    Document::Ptr doc = Document::create("typeTemplateParameterWithDefault");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QVERIFY(doc->globalSymbolCount()>=2);

    Snapshot snapshot;
    snapshot.insert(doc);

    {   // Test "S::value"
        Class *x = doc->globalSymbolAt(0)->asClass();
        QVERIFY(x);
        QCOMPARE(x->name()->identifier()->chars(), "X");
        QCOMPARE(x->memberCount(), 1);

        Class *s = doc->globalSymbolAt(1)->asClass();
        QVERIFY(s);
        QCOMPARE(s->name()->identifier()->chars(), "S");
        QCOMPARE(s->memberCount(), 1);

        Declaration *xv = x->memberAt(0)->asDeclaration();
        QVERIFY(xv);
        QCOMPARE(xv->name()->identifier()->chars(), "value");

        Declaration *sv = s->memberAt(0)->asDeclaration();
        QVERIFY(sv);
        QCOMPARE(sv->name()->identifier()->chars(), "value");

        FindUsages find(src, doc, snapshot);
        find(xv);
        QCOMPARE(find.usages().size(), 2);
        find(sv);
        QCOMPARE(find.usages().size(), 3);
    }
}

void tst_FindUsages::resolveOrder_for_templateFunction_vs_function()
{
    const QByteArray src = "struct X{int value;};\n"
                           "struct S{int value;};\n"
                           "X foo(){return X();}\n"
                           "template<class T = S> T foo(){return T();}\n"
                           "int main(){\n"
                           "    foo().value;\n"     // this is X.value
                           "}";

    Document::Ptr doc = Document::create("resolveOrder_for_templateFunction_vs_function");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QVERIFY(doc->globalSymbolCount()>=1);

    Snapshot snapshot;
    snapshot.insert(doc);

    {   // Test "S::value"
        Class *x = doc->globalSymbolAt(0)->asClass();
        QVERIFY(x);
        QCOMPARE(x->name()->identifier()->chars(), "X");
        QCOMPARE(x->memberCount(), 1);

        Declaration *xv = x->memberAt(0)->asDeclaration();
        QVERIFY(xv);
        QCOMPARE(xv->name()->identifier()->chars(), "value");

        FindUsages find(src, doc, snapshot);
        find(xv);
        QCOMPARE(find.usages().size(), 2);
    }
}

void tst_FindUsages::templateArrowOperator_with_defaultType()
{
    const QByteArray src = "struct S{int value;};\n"
                           "struct C{\n"
                           "    S* s;\n"
                           "    template<class T = S> \n"
                           "    T* operator->(){return &s;}\n"
                           "};\n"
                           "int main(){\n"
                           "    C().operator -> ()->value;\n"
                           "    C()->value;\n"
                           "}\n";

    Document::Ptr doc = Document::create("templateArrowOperator_with_defaultType");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QVERIFY(doc->globalSymbolCount()>=1);

    Snapshot snapshot;
    snapshot.insert(doc);

    {   // Test "S::value"
        Class *s = doc->globalSymbolAt(0)->asClass();
        QVERIFY(s);
        QCOMPARE(s->name()->identifier()->chars(), "S");
        QCOMPARE(s->memberCount(), 1);

        Declaration *sv = s->memberAt(0)->asDeclaration();
        QVERIFY(sv);
        QCOMPARE(sv->name()->identifier()->chars(), "value");

        FindUsages find(src, doc, snapshot);
        find(sv);
        QCOMPARE(find.usages().size(), 3);
    }
}

void tst_FindUsages::templateSpecialization_with_IntArgument()
{
    const QByteArray src = "\n"
                           "struct S0{ int value = 0; };\n"
                           "struct S1{ int value = 1; };\n"
                           "struct S2{ int value = 2; };\n"
                           "template<int N> struct S { S0 s; };\n"
                           "template<> struct S<1> { S1 s; };\n"
                           "template<> struct S<2> { S2 s; };\n"
                           "int main()\n"
                           "{\n"
                           "    S<0> s0;\n"
                           "    S<1> s1;\n"
                           "    S<2> s2;\n"
                           "    s0.s.value;\n"
                           "    s1.s.value;\n"
                           "    s2.s.value;\n"
                           "}\n";

    Document::Ptr doc = Document::create("templateSpecialization_with_IntArgument");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QVERIFY(doc->globalSymbolCount()>=3);

    Snapshot snapshot;
    snapshot.insert(doc);

    {
        Class *s[3] = {
            doc->globalSymbolAt(0)->asClass(),
            doc->globalSymbolAt(1)->asClass(),
            doc->globalSymbolAt(2)->asClass(),
        };

        QVERIFY(s[0]);
        QVERIFY(s[1]);
        QVERIFY(s[2]);

        QCOMPARE(s[0]->name()->identifier()->chars(), "S0");
        QCOMPARE(s[1]->name()->identifier()->chars(), "S1");
        QCOMPARE(s[2]->name()->identifier()->chars(), "S2");

        QCOMPARE(s[0]->memberCount(), 1);
        QCOMPARE(s[1]->memberCount(), 1);
        QCOMPARE(s[2]->memberCount(), 1);

        Declaration *sv[3] = {
            s[0]->memberAt(0)->asDeclaration(),
            s[1]->memberAt(0)->asDeclaration(),
            s[2]->memberAt(0)->asDeclaration(),
        };

        QVERIFY(sv[0]);
        QVERIFY(sv[1]);
        QVERIFY(sv[2]);

        QCOMPARE(sv[0]->name()->identifier()->chars(), "value");
        QCOMPARE(sv[1]->name()->identifier()->chars(), "value");
        QCOMPARE(sv[2]->name()->identifier()->chars(), "value");

        FindUsages find(src, doc, snapshot);

        find(sv[0]);
        QCOMPARE(find.usages().size(), 2);

        QCOMPARE(find.usages()[0].line, 1);
        QCOMPARE(find.usages()[0].col, 15);
        QCOMPARE(find.usages()[1].line, 12);
        QCOMPARE(find.usages()[1].col, 9);

        find(sv[1]);
        QCOMPARE(find.usages().size(), 2);

        QCOMPARE(find.usages()[0].line, 2);
        QCOMPARE(find.usages()[0].col, 15);
        QCOMPARE(find.usages()[1].line, 13);
        QCOMPARE(find.usages()[1].col, 9);

        find(sv[2]);
        QCOMPARE(find.usages().size(), 2);

        QCOMPARE(find.usages()[0].line, 3);
        QCOMPARE(find.usages()[0].col, 15);
        QCOMPARE(find.usages()[1].line, 14);
        QCOMPARE(find.usages()[1].col, 9);
    }
}

void tst_FindUsages::templateSpecialization_with_BoolArgument()
{
    const QByteArray src = "\n"
                           "struct S0{ int value = 0; };\n"
                           "struct S1{ int value = 1; };\n"
                           "template<bool B> struct S { S0 s; };\n"
                           "template<> struct S<true> { S1 s; };\n"
                           "int main()\n"
                           "{\n"
                           "    S<false> s0;\n"
                           "    S<true> s1;\n"
                           "    s0.s.value;\n"
                           "    s1.s.value;\n"
                           "}\n";

    Document::Ptr doc = Document::create("templateSpecialization_with_BoolArgument");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QVERIFY(doc->globalSymbolCount()>=3);

    Snapshot snapshot;
    snapshot.insert(doc);

    {
        Class *s[2] = {
            doc->globalSymbolAt(0)->asClass(),
            doc->globalSymbolAt(1)->asClass(),
        };

        QVERIFY(s[0]);
        QVERIFY(s[1]);

        QCOMPARE(s[0]->name()->identifier()->chars(), "S0");
        QCOMPARE(s[1]->name()->identifier()->chars(), "S1");

        QCOMPARE(s[0]->memberCount(), 1);
        QCOMPARE(s[1]->memberCount(), 1);

        Declaration *sv[2] = {
            s[0]->memberAt(0)->asDeclaration(),
            s[1]->memberAt(0)->asDeclaration(),
        };

        QVERIFY(sv[0]);
        QVERIFY(sv[1]);

        QCOMPARE(sv[0]->name()->identifier()->chars(), "value");
        QCOMPARE(sv[1]->name()->identifier()->chars(), "value");

        FindUsages find(src, doc, snapshot);

        find(sv[0]);
        QCOMPARE(find.usages().size(), 2);

        QCOMPARE(find.usages()[0].line, 1);
        QCOMPARE(find.usages()[0].col, 15);
        QCOMPARE(find.usages()[1].line, 9);
        QCOMPARE(find.usages()[1].col, 9);

        find(sv[1]);
        QCOMPARE(find.usages().size(), 2);

        QCOMPARE(find.usages()[0].line, 2);
        QCOMPARE(find.usages()[0].col, 15);
        QCOMPARE(find.usages()[1].line, 10);
        QCOMPARE(find.usages()[1].col, 9);
    }
}

void tst_FindUsages::templatePartialSpecialization()
{
    const QByteArray src = "\n"
                           "struct S0{ int value = 0; };\n"
                           "struct S1{ int value = 1; };\n"
                           "template<class T, class U> struct S { S0 ss; };\n"
                           "template<class U> struct S<float, U> { S1 ss; };\n"
                           "int main()\n"
                           "{\n"
                           "    S<int, int> s0;\n"
                           "    S<float, int> s1;\n"
                           "    s0.ss.value;\n"
                           "    s1.ss.value;\n"
                           "}\n";

    Document::Ptr doc = Document::create("templatePartialSpecialization");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QVERIFY(doc->globalSymbolCount()>=3);

    Snapshot snapshot;
    snapshot.insert(doc);

    {
        Class *s[2] = {
            doc->globalSymbolAt(0)->asClass(),
            doc->globalSymbolAt(1)->asClass(),
        };

        QVERIFY(s[0]);
        QVERIFY(s[1]);

        QCOMPARE(s[0]->name()->identifier()->chars(), "S0");
        QCOMPARE(s[1]->name()->identifier()->chars(), "S1");

        QCOMPARE(s[0]->memberCount(), 1);
        QCOMPARE(s[1]->memberCount(), 1);

        Declaration *sv[2] = {
            s[0]->memberAt(0)->asDeclaration(),
            s[1]->memberAt(0)->asDeclaration(),
        };

        QVERIFY(sv[0]);
        QVERIFY(sv[1]);

        QCOMPARE(sv[0]->name()->identifier()->chars(), "value");
        QCOMPARE(sv[1]->name()->identifier()->chars(), "value");

        FindUsages find(src, doc, snapshot);

        find(sv[0]);
        QCOMPARE(find.usages().size(), 2);

        QCOMPARE(find.usages()[0].line, 1);
        QCOMPARE(find.usages()[0].col, 15);
        QCOMPARE(find.usages()[1].line, 9);
        QCOMPARE(find.usages()[1].col, 10);

        find(sv[1]);
        QCOMPARE(find.usages().size(), 2);

        QCOMPARE(find.usages()[0].line, 2);
        QCOMPARE(find.usages()[0].col, 15);
        QCOMPARE(find.usages()[1].line, 10);
        QCOMPARE(find.usages()[1].col, 10);
    }
}

void tst_FindUsages::templatePartialSpecialization_2()
{
    const QByteArray src =
R"(
struct S0{int value=0;};
struct S1{int value=1;};
struct S2{int value=2;};
template<class T1, class T2> struct S{T1 ss;};
template<class U> struct S<int, U>{ U ss; };
template<class V> struct S<V*, int>{ V *ss; };
int main()
{
    S<S0, float> s0;
    s0.ss.value;
    S<int, S1> s1;
    s1.ss.value;
    S<S2*, int> s2;
    s2.ss->value;
}
)";

    Document::Ptr doc = Document::create("templatePartialSpecialization_2");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QVERIFY(doc->globalSymbolCount()>=3);

    Snapshot snapshot;
    snapshot.insert(doc);

    FindUsages find(src, doc, snapshot);

    Class *s[3];
    Declaration *sv[3];
    for (int i = 0; i < 3; i++) {
        s[i] = doc->globalSymbolAt(i)->asClass();
        QVERIFY(s[i]);
        QCOMPARE(s[i]->memberCount(), 1);
        sv[i] = s[i]->memberAt(0)->asDeclaration();
        QVERIFY(sv[i]);
        QCOMPARE(sv[i]->name()->identifier()->chars(), "value");
    }
    QCOMPARE(s[0]->name()->identifier()->chars(), "S0");
    QCOMPARE(s[1]->name()->identifier()->chars(), "S1");
    QCOMPARE(s[2]->name()->identifier()->chars(), "S2");

    find(sv[0]);
    QCOMPARE(find.usages().size(), 2);

    find(sv[1]);
    QCOMPARE(find.usages().size(), 2);

    find(sv[2]);
    QCOMPARE(find.usages().size(), 2);
}

void tst_FindUsages::template_SFINAE_1()
{
    const QByteArray src =
R"(
struct S{int value=1;};
template<class, class> struct is_same {};
template<class T> struct is_same<T, T> {using type = int;};
template<class T = S, typename is_same<T, S>::type = 0> T* foo(){return new T();}
int main(){
    foo()->value;
}
)";

    Document::Ptr doc = Document::create("template_SFINAE_1");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QVERIFY(doc->globalSymbolCount()>=1);

    Snapshot snapshot;
    snapshot.insert(doc);

    Class *s = doc->globalSymbolAt(0)->asClass();
    QVERIFY(s);
    QCOMPARE(s->name()->identifier()->chars(), "S");
    QCOMPARE(s->memberCount(), 1);

    Declaration *sv = s->memberAt(0)->asDeclaration();
    QVERIFY(sv);
    QCOMPARE(sv->name()->identifier()->chars(), "value");

    FindUsages find(src, doc, snapshot);
    find(sv);
    QCOMPARE(find.usages().size(), 2);
}

void tst_FindUsages::variableTemplateInExpression()
{
    const QByteArray src =
R"(
struct S{int value;};
template<class T> constexpr int foo = sizeof(T);
template<class T1, class T2>
struct pair{
    pair() noexcept(foo<T1> + foo<T2>){}
    T1 first;
    T2 second;
};
int main(){
    pair<int, S> pair;
    pair.second.value;
}
)";

    Document::Ptr doc = Document::create("variableTemplateInExpression");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QVERIFY(doc->globalSymbolCount()>=1);

    Snapshot snapshot;
    snapshot.insert(doc);

    Class *s = doc->globalSymbolAt(0)->asClass();
    QVERIFY(s);
    QCOMPARE(s->name()->identifier()->chars(), "S");
    QCOMPARE(s->memberCount(), 1);

    Declaration *sv = s->memberAt(0)->asDeclaration();
    QVERIFY(sv);
    QCOMPARE(sv->name()->identifier()->chars(), "value");

    FindUsages find(src, doc, snapshot);
    find(sv);
    QCOMPARE(find.usages().size(), 2);
}

void tst_FindUsages::variadicMacros()
{
    const QByteArray src =
        R"(
struct MyStruct { int value; };
#define FOO( ... ) int foo()
FOO(1) {
    MyStruct s;
    s.value;
}
int main(){}
)";

    Document::Ptr doc = Document::create("variadicMacros");
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QVERIFY(doc->globalSymbolCount() >= 1);

    Snapshot snapshot;
    snapshot.insert(doc);

    Class *s = doc->globalSymbolAt(0)->asClass();
    QVERIFY(s);
    QCOMPARE(s->name()->identifier()->chars(), "MyStruct");
    QCOMPARE(s->memberCount(), 1);

    Declaration *sv = s->memberAt(0)->asDeclaration();
    QVERIFY(sv);
    QCOMPARE(sv->name()->identifier()->chars(), "value");

    FindUsages find(src, doc, snapshot);
    find(sv);
    QCOMPARE(find.usages().size(), 2);
}

QTEST_APPLESS_MAIN(tst_FindUsages)
#include "tst_findusages.moc"
