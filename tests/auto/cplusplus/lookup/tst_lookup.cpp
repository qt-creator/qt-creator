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

#include <QtTest>
#include <QObject>

#include <cplusplus/AST.h>
#include <cplusplus/ASTVisitor.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/Literals.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Name.h>
#include <cplusplus/NamePrettyPrinter.h>
#include <cplusplus/Overview.h>
#include <cplusplus/ResolveExpression.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/TranslationUnit.h>

//TESTED_COMPONENT=src/libs/cplusplus
using namespace CPlusPlus;

template <template <typename, typename> class _Map, typename _T1, typename _T2>
_Map<_T2, _T1> invert(const _Map<_T1, _T2> &m)
{
    _Map<_T2, _T1> i;
    typename _Map<_T1, _T2>::const_iterator it = m.constBegin();
    for (; it != m.constEnd(); ++it) {
        i.insertMulti(it.value(), it.key());
    }
    return i;
}

class ClassSymbols: protected ASTVisitor,
    public QMap<ClassSpecifierAST *, Class *>
{
public:
    ClassSymbols(TranslationUnit *translationUnit)
        : ASTVisitor(translationUnit)
    { }

    QMap<ClassSpecifierAST *, Class *> asMap() const
    { return *this; }

    void operator()(AST *ast)
    { accept(ast); }

protected:
    virtual bool visit(ClassSpecifierAST *ast)
    {
        Class *classSymbol = ast->symbol;
        Q_ASSERT(classSymbol != 0);

        insert(ast, classSymbol);

        return true;
    }
};

class tst_Lookup: public QObject
{
    Q_OBJECT

private slots:
    void base_class_defined_1();

    void document_functionAt_data();
    void document_functionAt();

    // Objective-C
    void simple_class_1();
    void class_with_baseclass();
    void class_with_protocol_with_protocol();
    void iface_impl_scoping();

    // template instantiation:
    void templates_1();
    void templates_2();
    void templates_3();
    void templates_4();
    void templates_5();

    void minimalname_data();
    void minimalname();
};

void tst_Lookup::base_class_defined_1()
{
    Overview overview;

    const QByteArray source = "\n"
        "class base {};\n"
        "class derived: public base {};\n";

    Document::Ptr doc = Document::create("base_class_defined_1");
    doc->setUtf8Source(source);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 2U);

    Snapshot snapshot;
    snapshot.insert(doc);

    Class *baseClass = doc->globalSymbolAt(0)->asClass();
    QVERIFY(baseClass);

    Class *derivedClass = doc->globalSymbolAt(1)->asClass();
    QVERIFY(derivedClass);

    const LookupContext ctx(doc, snapshot);

    ClassOrNamespace *klass = ctx.lookupType(derivedClass->baseClassAt(0)->name(), derivedClass->enclosingScope());
    QVERIFY(klass != 0);

    QCOMPARE(klass->symbols().size(), 1);
    QCOMPARE(klass->symbols().first(), baseClass);

    TranslationUnit *unit = doc->translationUnit();
    QVERIFY(unit != 0);

    TranslationUnitAST *ast = unit->ast()->asTranslationUnit();
    QVERIFY(ast != 0);

    ClassSymbols classSymbols(unit);
    classSymbols(ast);

    QCOMPARE(classSymbols.size(), 2);

    const QMap<Class *, ClassSpecifierAST *> classToAST =
            invert(classSymbols.asMap());

    QVERIFY(classToAST.value(baseClass) != 0);
    QVERIFY(classToAST.value(derivedClass) != 0);
}

void tst_Lookup::document_functionAt_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("column");
    QTest::addColumn<QString>("expectedFunction");
    QTest::addColumn<int>("expectedOpeningDeclaratorParenthesisLine");
    QTest::addColumn<int>("expectedClosingBraceLine");

    QByteArray source = "\n"
            "void Foo::Bar() {\n" // line 1
            "    \n"
            "    for (int i=0; i < 10; ++i) {\n" // line 3
            "        \n"
            "    }\n" // line 5
            "}\n";
    QString expectedFunction = QString::fromLatin1("Foo::Bar");
    QTest::newRow("nonInline1") << source << 1 << 2 << QString() << -1 << -1;
    QTest::newRow("nonInline2") << source << 1 << 11 << expectedFunction << 1 << 6;
    QTest::newRow("nonInline3") << source << 2 << 2 << expectedFunction << 1 << 6;
    QTest::newRow("nonInline4") << source << 3 << 10 << expectedFunction << 1 << 6;
    QTest::newRow("nonInline5") << source << 4 << 3 << expectedFunction << 1 << 6;
    QTest::newRow("nonInline6") << source << 6 << 1 << expectedFunction << 1 << 6;

    source = "\n"
            "namespace N {\n" // line 1
            "class C {\n"
            "    void f()\n" // line 3
            "    {\n"
            "    }\n" // line 5
            "};\n"
            "}\n"; // line 7
    expectedFunction = QString::fromLatin1("N::C::f");
    QTest::newRow("inline1") << source << 1 << 2 << QString() << -1 << -1;
    QTest::newRow("inline2") << source << 2 << 10 << QString() << -1 << -1;
    QTest::newRow("inline2") << source << 3 << 10 << expectedFunction << 3 << 5;

    source = "\n"
            "void f(Helper helper = [](){})\n" // line 1
            "{\n"
            "}\n"; // line 3
    expectedFunction = QString::fromLatin1("f");
    QTest::newRow("inlineWithLambdaArg1") << source << 2 << 1 << expectedFunction << 1 << 3;
}

void tst_Lookup::document_functionAt()
{
    QFETCH(QByteArray, source);
    QFETCH(int, line);
    QFETCH(int, column);
    QFETCH(QString, expectedFunction);
    QFETCH(int, expectedOpeningDeclaratorParenthesisLine);
    QFETCH(int, expectedClosingBraceLine);

    Document::Ptr doc = Document::create("document_functionAt");
    doc->setUtf8Source(source);
    doc->parse();
    doc->check();
    QVERIFY(doc->diagnosticMessages().isEmpty());

    int actualOpeningDeclaratorParenthesisLine = -1;
    int actualClosingBraceLine = -1;
    const QString actualFunction = doc->functionAt(line, column,
                                                   &actualOpeningDeclaratorParenthesisLine,
                                                   &actualClosingBraceLine);
    QCOMPARE(actualFunction, expectedFunction);
    QCOMPARE(actualOpeningDeclaratorParenthesisLine, expectedOpeningDeclaratorParenthesisLine);
    QCOMPARE(actualClosingBraceLine, expectedClosingBraceLine);
}

void tst_Lookup::simple_class_1()
{
    const QByteArray source = "\n"
        "@interface Zoo {} +(id)alloc; -(id)init; @end\n"
        "@implementation Zoo +(id)alloc{} -(id)init{} -(void)dealloc{} @end\n";

    Document::Ptr doc = Document::create("simple_class_1");
    doc->setUtf8Source(source);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 2U);

    Snapshot snapshot;
    snapshot.insert(doc);

    ObjCClass *iface = doc->globalSymbolAt(0)->asObjCClass();
    QVERIFY(iface);
    QVERIFY(iface->isInterface());
    QCOMPARE(iface->memberCount(), 2U);

    ObjCClass *impl = doc->globalSymbolAt(1)->asObjCClass();
    QVERIFY(impl);
    QVERIFY(!impl->isInterface());
    QCOMPARE(impl->memberCount(), 3U);

    Declaration *allocMethodIface = iface->memberAt(0)->asDeclaration();
    QVERIFY(allocMethodIface);
    QVERIFY(allocMethodIface->name() && allocMethodIface->name()->identifier());
    QCOMPARE(QLatin1String(allocMethodIface->name()->identifier()->chars()), QLatin1String("alloc"));

    ObjCMethod *allocMethodImpl = impl->memberAt(0)->asObjCMethod();
    QVERIFY(allocMethodImpl);
    QVERIFY(allocMethodImpl->name() && allocMethodImpl->name()->identifier());
    QCOMPARE(QLatin1String(allocMethodImpl->name()->identifier()->chars()), QLatin1String("alloc"));

    ObjCMethod *deallocMethod = impl->memberAt(2)->asObjCMethod();
    QVERIFY(deallocMethod);
    QVERIFY(deallocMethod->name() && deallocMethod->name()->identifier());
    QCOMPARE(QLatin1String(deallocMethod->name()->identifier()->chars()), QLatin1String("dealloc"));

    const LookupContext context(doc, snapshot);

    // check class resolving:
    ClassOrNamespace *klass = context.lookupType(impl->name(), impl->enclosingScope());
    QVERIFY(klass != 0);
    QCOMPARE(klass->symbols().size(), 2);
    QVERIFY(klass->symbols().contains(iface));
    QVERIFY(klass->symbols().contains(impl));

    // check method resolving:
    QList<LookupItem> results = context.lookup(allocMethodImpl->name(), impl);
    QCOMPARE(results.size(), 2);
    QCOMPARE(results.at(0).declaration(), allocMethodIface);
    QCOMPARE(results.at(1).declaration(), allocMethodImpl);

    results = context.lookup(deallocMethod->name(), impl);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0).declaration(), deallocMethod);
}

void tst_Lookup::class_with_baseclass()
{
    const QByteArray source = "\n"
                              "@implementation BaseZoo {} -(void)baseDecl; -(void)baseMethod{} @end\n"
                              "@interface Zoo: BaseZoo {} +(id)alloc; -(id)init; @end\n"
                              "@implementation Zoo +(id)alloc{} -(id)init{} -(void)dealloc{} @end\n";

    Document::Ptr doc = Document::create("class_with_baseclass");
    doc->setUtf8Source(source);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 3U);

    Snapshot snapshot;
    snapshot.insert(doc);

    Document::Ptr emptyDoc = Document::create("<empty>");

    ObjCClass *baseZoo = doc->globalSymbolAt(0)->asObjCClass();
    QVERIFY(baseZoo);
    QVERIFY(!baseZoo->isInterface());
    QCOMPARE(baseZoo->memberCount(), 2U);

    ObjCClass *zooIface = doc->globalSymbolAt(1)->asObjCClass();
    QVERIFY(zooIface);
    QVERIFY(zooIface->isInterface());
    QVERIFY(zooIface->baseClass()->name() == baseZoo->name());

    ObjCClass *zooImpl = doc->globalSymbolAt(2)->asObjCClass();
    QVERIFY(zooImpl);
    QVERIFY(!zooImpl->isInterface());
    QCOMPARE(zooImpl->memberCount(), 3U);

    Declaration *baseDecl = baseZoo->memberAt(0)->asDeclaration();
    QVERIFY(baseDecl);
    QVERIFY(baseDecl->name() && baseDecl->name()->identifier());
    QCOMPARE(QLatin1String(baseDecl->name()->identifier()->chars()), QLatin1String("baseDecl"));

    ObjCMethod *baseMethod = baseZoo->memberAt(1)->asObjCMethod();
    QVERIFY(baseMethod);
    QVERIFY(baseMethod->name() && baseMethod->name()->identifier());
    QCOMPARE(QLatin1String(baseMethod->name()->identifier()->chars()), QLatin1String("baseMethod"));

    const LookupContext context(doc, snapshot);

    ClassOrNamespace *objClass = context.lookupType(baseZoo->name(), zooImpl->enclosingScope());
    QVERIFY(objClass != 0);
    QVERIFY(objClass->symbols().contains(baseZoo));

    QList<LookupItem> results = context.lookup(baseDecl->name(), zooImpl);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0).declaration(), baseDecl);

    results = context.lookup(baseMethod->name(), zooImpl);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0).declaration(), baseMethod);
}

void tst_Lookup::class_with_protocol_with_protocol()
{
    const QByteArray source = "\n"
                              "@protocol P1 -(void)p1method; @end\n"
                              "@protocol P2 <P1> -(void)p2method; @end\n"
                              "@interface Zoo <P2> {} +(id)alloc; -(id)init; @end\n"
                              "@implementation Zoo +(id)alloc{} -(id)init{} -(void)dealloc{} @end\n";

    Document::Ptr doc = Document::create("class_with_protocol_with_protocol");
    doc->setUtf8Source(source);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 4U);

    Snapshot snapshot;
    snapshot.insert(doc);

    ObjCProtocol *P1 = doc->globalSymbolAt(0)->asObjCProtocol();
    QVERIFY(P1);
    QCOMPARE(P1->memberCount(), 1U);
    QCOMPARE(P1->protocolCount(), 0U);

    Declaration *p1method = P1->memberAt(0)->asDeclaration();
    QVERIFY(p1method);
    QCOMPARE(QLatin1String(p1method->name()->identifier()->chars()), QLatin1String("p1method"));

    ObjCProtocol *P2 = doc->globalSymbolAt(1)->asObjCProtocol();
    QVERIFY(P2);
    QCOMPARE(P2->memberCount(), 1U);
    QCOMPARE(P2->protocolCount(), 1U);
    QCOMPARE(QLatin1String(P2->protocolAt(0)->name()->identifier()->chars()), QLatin1String("P1"));

    ObjCClass *zooImpl = doc->globalSymbolAt(3)->asObjCClass();
    QVERIFY(zooImpl);

    const LookupContext context(doc, snapshot);

    {
        const QList<LookupItem> candidates = context.lookup(P1->name(), zooImpl->enclosingScope());
        QCOMPARE(candidates.size(), 1);
        QVERIFY(candidates.at(0).declaration() == P1);
    }

    {
        const QList<LookupItem> candidates = context.lookup(P2->protocolAt(0)->name(), zooImpl->enclosingScope());
        QCOMPARE(candidates.size(), 1);
        QVERIFY(candidates.first().declaration() == P1);
    }

    QList<LookupItem> results = context.lookup(p1method->name(), zooImpl);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0).declaration(), p1method);
}

void tst_Lookup::iface_impl_scoping()
{
    const QByteArray source = "\n"
                              "@interface Scooping{}-(int)method1:(int)arg;-(void)method2;@end\n"
                              "@implementation Scooping-(int)method1:(int)arg{return arg;}@end\n";

    Document::Ptr doc = Document::create("class_with_protocol_with_protocol");
    doc->setUtf8Source(source);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 2U);

    Snapshot snapshot;
    snapshot.insert(doc);

    ObjCClass *iface = doc->globalSymbolAt(0)->asObjCClass();
    QVERIFY(iface);
    QVERIFY(iface->isInterface());
    ObjCClass *impl = doc->globalSymbolAt(1)->asObjCClass();
    QVERIFY(impl);
    QVERIFY(!impl->isInterface());

    QCOMPARE(iface->memberCount(), 2U);
    QCOMPARE(impl->memberCount(), 1U);

    ObjCMethod *method1Impl = impl->memberAt(0)->asObjCMethod();
    QVERIFY(method1Impl);
    QCOMPARE(method1Impl->identifier()->chars(), "method1");

    // get the body of method1
    QCOMPARE(method1Impl->memberCount(), 2U);
    Argument *method1Arg = method1Impl->memberAt(0)->asArgument();
    QVERIFY(method1Arg);
    QCOMPARE(method1Arg->identifier()->chars(), "arg");
    QVERIFY(method1Arg->type()->isIntegerType());

    Block *method1Body = method1Impl->memberAt(1)->asBlock();
    QVERIFY(method1Body);

    const LookupContext context(doc, snapshot);

    { // verify if we can resolve "arg" in the body
        QCOMPARE(method1Impl->argumentCount(), 1U);
        Argument *arg = method1Impl->argumentAt(0)->asArgument();
        QVERIFY(arg);
        QVERIFY(arg->name());
        QVERIFY(arg->name()->identifier());
        QCOMPARE(arg->name()->identifier()->chars(), "arg");
        QVERIFY(arg->type()->isIntegerType());

        const QList<LookupItem> candidates = context.lookup(arg->name(), method1Body->enclosingScope());
        QCOMPARE(candidates.size(), 1);
        QVERIFY(candidates.at(0).declaration()->type()->asIntegerType());
    }

    Declaration *method2 = iface->memberAt(1)->asDeclaration();
    QVERIFY(method2);
    QCOMPARE(method2->identifier()->chars(), "method2");

    { // verify if we can resolve "method2" in the body
        const QList<LookupItem> candidates = context.lookup(method2->name(), method1Body->enclosingScope());
        QCOMPARE(candidates.size(), 1);
        QCOMPARE(candidates.at(0).declaration(), method2);
    }
}

void tst_Lookup::templates_1()
{
    const QByteArray source = "\n"
            "namespace std {\n"
            "    template <typename T>\n"
            "    struct _List_iterator {\n"
            "        T data;\n"
            "    };\n"
            "\n"
            "    template <typename T>\n"
            "    struct list {\n"
            "        typedef _List_iterator<T> iterator;\n"
            "\n"
            "        iterator begin();\n"
            "        _List_iterator<T> end();\n"
            "    };\n"
            "}\n"
            "\n"
            "struct Point {\n"
            "    int x, y;\n"
            "};\n"
            "\n"
            "int main()\n"
            "{\n"
            "    std::list<Point> l;\n"
            "    l.begin();  // std::_List_iterator<Point> .. and not only _List_iterator<Point>\n"
            "    l.end(); // std::_List_iterator<Point>\n"
            "}\n";
    Document::Ptr doc = Document::create("templates_1");
    doc->setUtf8Source(source);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
}

void tst_Lookup::templates_2()
{
    const QByteArray source = "\n"
            "template <typename T1>\n"
            "struct Node {\n"
            "    T1 value;\n"
            "    Node *next;\n"
            "    Node<T1> *other_next;\n"
            "};\n"
            "\n"
            "template <typename T2>\n"
            "struct List {\n"
            "    Node<T2> *elements;\n"
            "};\n"
            "\n"
            "int main()\n"
            "{\n"
            "    List<int> *e;\n"
            "    e->elements; // Node<int> *\n"
            "    e->elements->next; // Node<int> *\n"
            "    e->elements->other_next; // Node<int> *\n"
            "}\n"
;
    Document::Ptr doc = Document::create("templates_2");
    doc->setUtf8Source(source);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
}

void tst_Lookup::templates_3()
{
    const QByteArray source = "\n"
            "struct Point {\n"
            "    int x, y;\n"
            "};\n"
            "\n"
            "template <typename T = Point>\n"
            "struct List {\n"
            "    const T &at(int);\n"
            "};\n"
            "\n"
            "int main()\n"
            "{\n"
            "    List<> l;\n"
            "    l.at(0); // const Point &\n"
            "}\n";
    Document::Ptr doc = Document::create("templates_3");
    doc->setUtf8Source(source);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
}

void tst_Lookup::templates_4()
{
    const QByteArray source = "\n"
            "template <typename T>\n"
            "struct Allocator {\n"
            "    typedef T *pointer_type;\n"
            "    typedef T &reference_type;\n"
            "};\n"
            "\n"
            "template <typename T>\n"
            "struct SharedPtr {\n"
            "    typedef typename Allocator<T>::pointer_type pointer_type;\n"
            "    typedef typename Allocator<T>::reference_type reference_type;\n"
            "\n"
            "    pointer_type operator->();\n"
            "    reference_type operator*();\n"
            "\n"
            "    pointer_type data();\n"
            "    reference_type get();\n"
            "\n"
            "};\n"
            "\n"
            "struct Point {\n"
            "    int x,y;\n"
            "};\n"
            "\n"
            "int main()\n"
            "{\n"
            "    SharedPtr<Point> l;\n"
            "\n"
            "    l->x; // int\n"
            "    (*l); // Point &\n"
            "}\n";
    Document::Ptr doc = Document::create("templates_4");
    doc->setUtf8Source(source);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
}

void tst_Lookup::templates_5()
{
    const QByteArray source = "\n"
            "struct Point {\n"
            "    int x,y;\n"
            "};\n"
            "\n"
            "template <typename _Tp>\n"
            "struct Allocator {\n"
            "    typedef const _Tp &const_reference;\n"
            "\n"
            "    const_reference get();\n"
            "};\n"
            "\n"
            "int main()\n"
            "{\n"
            "    Allocator<Point>::const_reference r = pt;\n"
            "    //r.; // const Point &\n"
            "\n"
            "    Allocator<Point> a;\n"
            "    a.get(); // const Point &\n"
            "}\n";
    Document::Ptr doc = Document::create("templates_5");
    doc->setUtf8Source(source);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
}

void tst_Lookup::minimalname_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<int>("index");

    QTest::newRow("inlineNamespace1")
            << QByteArray("namespace std { inline namespace __cxx11 { class string{}; } }\n")
            << 0;

    // This case is extracted from libstdc++ 5.4.0.
    // The inline namespace is re-opened as non-inline, which is not standard
    // compliant. However, gcc does this and clang only issues a warning.
    QTest::newRow("inlineNamespace2")
            << QByteArray("namespace std { inline namespace __cxx11 {} }\n"
                          "namespace std { namespace __cxx11 { class string{}; } }\n")
            << 1;
}

void tst_Lookup::minimalname()
{
    QFETCH(QByteArray, source);
    QFETCH(int, index);

    Document::Ptr doc = Document::create("minimalname");
    doc->setUtf8Source(source);
    doc->parse();
    doc->check();

    Snapshot snapshot;
    snapshot.insert(doc);
    LookupContext ctx(doc, snapshot);
    Control control;
    Symbol *symbol = doc->globalSymbolAt(unsigned(index))
            ->asNamespace()->memberAt(0)->asNamespace()->memberAt(0);

    const Name *minimalName = LookupContext::minimalName(symbol, ctx.globalNamespace(), &control);

    Overview oo;
    const QString minimalNameAsString = NamePrettyPrinter(&oo)(minimalName);
    QCOMPARE(minimalNameAsString, QString::fromUtf8("std::string"));
}

QTEST_APPLESS_MAIN(tst_Lookup)
#include "tst_lookup.moc"
