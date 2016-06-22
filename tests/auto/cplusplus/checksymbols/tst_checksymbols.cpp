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

#include <cplusplus/CppDocument.h>
#include <cplusplus/pp.h>

#include <cpptools/cppchecksymbols.h>
#include <cpptools/cppsemanticinfo.h>
#include <cpptools/cpptoolstestcase.h>
#include <texteditor/semantichighlighter.h>

#include <QDebug>
#include <QDir>
#include <QList>
#include <QtTest>

/*!
    Tests CheckSymbols, the "data provider" of the semantic highlighter.
 */

// When adding tests, you may want to set this enum
// in order to print out all found uses.
enum { enableListing = 0 };

using namespace CPlusPlus;
using namespace CppTools;

typedef QByteArray _;
typedef CheckSymbols::Result Use;
typedef CheckSymbols::Kind UseKind;
typedef SemanticHighlighter Highlighting;
typedef QList<Use> UseList;
Q_DECLARE_METATYPE(UseList)

#define CASE_STR(val) case Highlighting::val: return "Highlighting::" # val
static QString useKindToString(UseKind useKind)
{
    switch (useKind) {
    CASE_STR(Unknown);
    CASE_STR(TypeUse);
    CASE_STR(LocalUse);
    CASE_STR(FieldUse);
    CASE_STR(EnumerationUse);
    CASE_STR(VirtualMethodUse);
    CASE_STR(VirtualFunctionDeclarationUse);
    CASE_STR(LabelUse);
    CASE_STR(MacroUse);
    CASE_STR(FunctionUse);
    CASE_STR(FunctionDeclarationUse);
    CASE_STR(PseudoKeywordUse);
    CASE_STR(StringUse);
    default:
        QTest::qFail("Unknown UseKind", __FILE__, __LINE__);
        return QLatin1String("Unknown UseKind");
    }
}
#undef CASE_STR

// The following two functions are "enhancements" for QCOMPARE().
QT_BEGIN_NAMESPACE
namespace QTest {

template<>
char *toString(const Use &use)
{
    QByteArray ba = "Use(";
    ba += QByteArray::number(use.line);
    ba += ", " + QByteArray::number(use.column);
    ba += ", " + QByteArray::number(use.length);
    ba += ", " + useKindToString(static_cast<UseKind>(use.kind)).toLatin1();
    ba += ")";
    return qstrdup(ba.data());
}

} // namespace QTest
QT_END_NAMESPACE

namespace {

class BaseTestCase
{
public:
    BaseTestCase(const QByteArray &source, const UseList &expectedUsesMacros = UseList())
    {
        // Write source to temporary file
        const QString filePath = QDir::tempPath() + QLatin1String("/file.h");
        Tests::TestCase::writeFile(filePath, source);

        // Process source
        const Document::Ptr document = createDocument(filePath, source);
        Snapshot snapshot;
        snapshot.insert(document);

        // Collect symbols
        future = runCheckSymbols(document, snapshot, expectedUsesMacros);
    }

    static CheckSymbols::Future runCheckSymbols(const Document::Ptr &document,
                                                const Snapshot &snapshot,
                                                const UseList &expectedUsesMacros = UseList())
    {
        LookupContext context(document, snapshot);
        CheckSymbols::Future future = CheckSymbols::go(document, context, expectedUsesMacros);
        future.waitForFinished();
        return future;
    }

    static Document::Ptr createDocument(const QString &filePath, const QByteArray &source)
    {
        Environment env;
        Preprocessor preprocess(0, &env);
        preprocess.setKeepComments(true);
        const QByteArray preprocessedSource = preprocess.run(filePath, source);

        Document::Ptr document = Document::create(filePath);
        document->setUtf8Source(preprocessedSource);
        if (!document->parse())
            return Document::Ptr();
        document->check();
        if (!document->diagnosticMessages().isEmpty())
            return Document::Ptr();

        return document;
    }

    Use findUse(unsigned line, unsigned column)
    {
        const int resultCount = future.resultCount();
        for (int i = resultCount - 1; i >= 0; --i) {
            Use result = future.resultAt(i);
            if (result.line > line)
                continue;
            if (result.line < line || result.column < column)
                break;
            if (result.column == column)
                return result;
        }
        return Use();
    }

    CheckSymbols::Future future;
};

class TestCase : public BaseTestCase
{
public:
    TestCase(const QByteArray &source,
             const UseList &expectedUsesAll,
             const UseList &expectedUsesMacros = UseList())
        : BaseTestCase(source, expectedUsesMacros)
    {
        const int resultCount = future.resultCount();
        UseList actualUses;
        QByteArray expectedInput;
        if (enableListing)
            expectedInput = _("\n") + _(8, ' ') + "<< (UseList()\n";
        for (int i = 0; i < resultCount; ++i) {
            const Use use = future.resultAt(i);
            if (enableListing)
                expectedInput += _(12, ' ') + "<< " + _(QTest::toString(use)) + "\n";
            actualUses.append(use);
        }

        if (enableListing) {
            expectedInput.chop(1);
            expectedInput += ')';
            qDebug() << expectedInput;
        }
        // Checks
        QVERIFY(resultCount > 0);
        QCOMPARE(resultCount, expectedUsesAll.count());

        for (int i = 0; i < resultCount; ++i) {
            const Use actualUse = actualUses.at(i);
            const Use expectedUse = expectedUsesAll.at(i);
            QVERIFY(actualUse.isValid());
            QVERIFY(expectedUse.isValid());
            QCOMPARE(actualUse, expectedUse);
        }
    }
};

} // anonymous namespace

class tst_CheckSymbols: public QObject
{
    Q_OBJECT

private slots:

    void test_checksymbols();
    void test_checksymbols_data();

    void test_checksymbols_macroUses();
    void test_checksymbols_macroUses_data();

    void test_checksymbols_infiniteLoop_data();
    void test_checksymbols_infiniteLoop();

    void test_checkForValidSymbolFileId();

    void test_parentOfBlock();
    void test_infiniteLoop();

    void findField();
    void findField_data();
};

void tst_CheckSymbols::test_checksymbols()
{
    QFETCH(QByteArray, source);
    QFETCH(UseList, expectedUsesAll);

    TestCase(source, expectedUsesAll);
}

void tst_CheckSymbols::test_checksymbols_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<UseList>("expectedUsesAll");

    QTest::newRow("TypeUse")
        << _("namespace N {}\n"
             "using namespace N;\n")
        << (UseList()
            << Use(1, 11, 1, Highlighting::TypeUse)
            << Use(2, 17, 1, Highlighting::TypeUse));

    QTest::newRow("LocalUse")
        << _("int f()\n"
             "{\n"
             "   int i;\n"
             "}\n")
        << (UseList()
            << Use(1, 5, 1, Highlighting::FunctionDeclarationUse)
            << Use(3, 8, 1, Highlighting::LocalUse));

    QTest::newRow("FieldUse")
        << _("struct F {\n"          // 1
             "    int i;\n"
             "    F() { i = 0; }\n"
             "};\n"
             "int f()\n"             // 5
             "{\n"
             "    F s;\n"
             "    s.i = 2;\n"
             "}\n")
        << (UseList()
            << Use(1, 8, 1, Highlighting::TypeUse)
            << Use(2, 9, 1, Highlighting::FieldUse)
            << Use(3, 5, 1, Highlighting::TypeUse)
            << Use(3, 11, 1, Highlighting::FieldUse)
            << Use(5, 5, 1, Highlighting::FunctionDeclarationUse)
            << Use(7, 5, 1, Highlighting::TypeUse)
            << Use(7, 7, 1, Highlighting::LocalUse)
            << Use(8, 5, 1, Highlighting::LocalUse)
            << Use(8, 7, 1, Highlighting::FieldUse));

    QTest::newRow("EnumerationUse")
        << _("enum E { Red, Green, Blue };\n"
             "E e = Red;\n")
        << (UseList()
            << Use(1, 6, 1, Highlighting::TypeUse)
            << Use(1, 10, 3, Highlighting::EnumerationUse)
            << Use(1, 15, 5, Highlighting::EnumerationUse)
            << Use(1, 22, 4, Highlighting::EnumerationUse)
            << Use(2, 1, 1, Highlighting::TypeUse)
            << Use(2, 7, 3, Highlighting::EnumerationUse));

    QTest::newRow("VirtualFunction")
        << _("class B {\n"                   // 1
             "    virtual bool isThere();\n" // 2
             "    bool nonVirtual() {\n"     // 3
             "        return isThere();\n"   // 4
             "    }\n"                       // 5
             "};\n"                          // 6
             "class D: public B {\n"         // 7
             "    bool isThere();\n"         // 8
             "};\n")
        << (UseList()
            << Use(1, 7, 1, Highlighting::TypeUse)              // B
            << Use(2, 18, 7, Highlighting::VirtualFunctionDeclarationUse)    // isThere
            << Use(3, 10, 10, Highlighting::FunctionDeclarationUse)    // nonVirtual
            << Use(4, 16, 7, Highlighting::VirtualMethodUse)    // isThere call
            << Use(7, 7, 1, Highlighting::TypeUse)              // D
            << Use(7, 17, 1, Highlighting::TypeUse)             // B
            << Use(8, 10, 7, Highlighting::VirtualFunctionDeclarationUse));  // isThere

    QTest::newRow("LabelUse")
        << _("int f()\n"
             "{\n"
             "   goto g;\n"
             "   g: return 1;\n"
             "}\n")
        << (UseList()
            << Use(1, 5, 1, Highlighting::FunctionDeclarationUse)
            << Use(3, 9, 1, Highlighting::LabelUse)
            << Use(4, 4, 1, Highlighting::LabelUse));

    QTest::newRow("FunctionUse")
        << _("int f();\n"
             "int g() { f(); }\n")
        << (UseList()
            << Use(1, 5, 1, Highlighting::FunctionDeclarationUse)
            << Use(2, 5, 1, Highlighting::FunctionDeclarationUse)
            << Use(2, 11, 1, Highlighting::FunctionUse));

    QTest::newRow("PseudoKeywordUse")
        << _("class D : public B {\n"
             "   virtual void f() override {}\n"
             "   virtual void f() final {}\n"
             "};\n")
        << (UseList()
            << Use(1, 7, 1, Highlighting::TypeUse)
            << Use(2, 17, 1, Highlighting::VirtualFunctionDeclarationUse)
            << Use(2, 21, 8, Highlighting::PseudoKeywordUse)
            << Use(3, 17, 1, Highlighting::VirtualFunctionDeclarationUse)
            << Use(3, 21, 5, Highlighting::PseudoKeywordUse));

    QTest::newRow("StaticUse")
        << _("struct Outer\n"
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
             "}\n")
        << (UseList()
            << Use(1, 8, 5, Highlighting::TypeUse)
            << Use(3, 16, 3, Highlighting::FieldUse)
            << Use(4, 12, 5, Highlighting::TypeUse)
            << Use(6, 9, 5, Highlighting::TypeUse)
            << Use(6, 16, 5, Highlighting::FieldUse)
            << Use(7, 14, 3, Highlighting::FunctionDeclarationUse)
            << Use(11, 5, 5, Highlighting::TypeUse)
            << Use(11, 12, 3, Highlighting::FieldUse)
            << Use(13, 6, 5, Highlighting::TypeUse)
            << Use(13, 13, 5, Highlighting::TypeUse)
            << Use(13, 20, 3, Highlighting::FunctionDeclarationUse)
            << Use(15, 5, 3, Highlighting::FieldUse)
            << Use(16, 5, 5, Highlighting::TypeUse)
            << Use(16, 12, 3, Highlighting::FieldUse)
            << Use(17, 5, 5, Highlighting::FieldUse)
            << Use(17, 12, 3, Highlighting::FieldUse));

    QTest::newRow("VariableHasTheSameNameAsEnumUse")
        << _("struct Foo\n"
             "{\n"
             "    enum E { bar, baz };\n"
             "};\n"
             "\n"
             "struct Boo\n"
             "{\n"
             "    int foo;\n"
             "    int bar;\n"
             "    int baz;\n"
             "};\n")
        << (UseList()
            << Use(1, 8, 3, Highlighting::TypeUse)
            << Use(3, 10, 1, Highlighting::TypeUse)
            << Use(3, 14, 3, Highlighting::EnumerationUse)
            << Use(3, 19, 3, Highlighting::EnumerationUse)
            << Use(6, 8, 3, Highlighting::TypeUse)
            << Use(8, 9, 3, Highlighting::FieldUse)
            << Use(9, 9, 3, Highlighting::FieldUse)
            << Use(10, 9, 3, Highlighting::FieldUse));

    QTest::newRow("NestedClassOfEnclosingTemplateUse")
        << _("struct Foo { int bar; };\n"
             "\n"
             "template<typename T>\n"
             "struct Outer\n"
             "{\n"
             "    struct Nested { T nt; } nested;\n"
             "};\n"
             "\n"
             "void fun()\n"
             "{\n"
             "    Outer<Foo> list;\n"
             "    list.nested.nt.bar;\n"
             "}\n")
        << (UseList()
            << Use(1, 8, 3, Highlighting::TypeUse)
            << Use(1, 18, 3, Highlighting::FieldUse)
            << Use(3, 19, 1, Highlighting::TypeUse)
            << Use(4, 8, 5, Highlighting::TypeUse)
            << Use(6, 12, 6, Highlighting::TypeUse)
            << Use(6, 21, 1, Highlighting::TypeUse)
            << Use(6, 23, 2, Highlighting::FieldUse)
            << Use(6, 29, 6, Highlighting::FieldUse)
            << Use(9, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(11, 5, 5, Highlighting::TypeUse)
            << Use(11, 11, 3, Highlighting::TypeUse)
            << Use(11, 16, 4, Highlighting::LocalUse)
            << Use(12, 5, 4, Highlighting::LocalUse)
            << Use(12, 10, 6, Highlighting::FieldUse)
            << Use(12, 17, 2, Highlighting::FieldUse)
            << Use(12, 20, 3, Highlighting::FieldUse));

    QTest::newRow("8902_staticFunctionHighlightingAsMember_localVariable")
        << _("struct Foo\n"
             "{\n"
             "    static int foo();\n"
             "};\n"
             "\n"
             "void bar()\n"
             "{\n"
             "    int foo = Foo::foo();\n"
             "}\n")
        << (UseList()
            << Use(1, 8, 3, Highlighting::TypeUse)
            << Use(3, 16, 3, Highlighting::FunctionDeclarationUse)
            << Use(6, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(8, 9, 3, Highlighting::LocalUse)
            << Use(8, 15, 3, Highlighting::TypeUse)
            << Use(8, 20, 3, Highlighting::FunctionUse));

    QTest::newRow("8902_staticFunctionHighlightingAsMember_functionArgument")
        << _("struct Foo\n"
             "{\n"
             "    static int foo();\n"
             "};\n"
             "\n"
             "void bar(int foo)\n"
             "{\n"
             "    Foo::foo();\n"
             "}\n")
        << (UseList()
            << Use(1, 8, 3, Highlighting::TypeUse)
            << Use(3, 16, 3, Highlighting::FunctionDeclarationUse)
            << Use(6, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(6, 14, 3, Highlighting::LocalUse)
            << Use(8, 5, 3, Highlighting::TypeUse)
            << Use(8, 10, 3, Highlighting::FunctionUse));

    QTest::newRow("8902_staticFunctionHighlightingAsMember_templateParameter")
        << _("struct Foo\n"
             "{\n"
             "    static int foo();\n"
             "};\n"
             "\n"
             "template <class foo>\n"
             "void bar()\n"
             "{\n"
             "    Foo::foo();\n"
             "}\n")
        << (UseList()
            << Use(1, 8, 3, Highlighting::TypeUse)
            << Use(3, 16, 3, Highlighting::FunctionDeclarationUse)
            << Use(6, 17, 3, Highlighting::TypeUse)
            << Use(7, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(9, 5, 3, Highlighting::TypeUse)
            << Use(9, 10, 3, Highlighting::FunctionUse));

    QTest::newRow("staticFunctionHighlightingAsMember_struct")
        << _("struct Foo\n"
             "{\n"
             "    static int foo();\n"
             "};\n"
             "\n"
             "struct foo {};\n"
             "void bar()\n"
             "{\n"
             "    Foo::foo();\n"
             "}\n")
        << (UseList()
            << Use(1, 8, 3, Highlighting::TypeUse)
            << Use(3, 16, 3, Highlighting::FunctionDeclarationUse)
            << Use(6, 8, 3, Highlighting::TypeUse)
            << Use(7, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(9, 5, 3, Highlighting::TypeUse)
            << Use(9, 10, 3, Highlighting::FunctionUse));

    QTest::newRow("QTCREATORBUG8890_danglingPointer")
        << _("template<class T> class QList {\n"
             "    public:\n"
             "        T operator[](int);\n"
             "};\n"
             "\n"
             "template<class T> class QPointer {\n"
             "    public:\n"
             "        T& operator->();\n"
             "};\n"
             "\n"
             "class Foo {\n"
             "    void foo() {}\n"
             "};\n"
             "\n"
             "void f()\n"
             "{\n"
             "    QList<QPointer<Foo> > list;\n"
             "    list[0]->foo();\n"
             "    list[0]->foo(); // Crashed because of this 'extra' line.\n"
             "}\n")
        << (UseList()
            << Use(1, 16, 1, Highlighting::TypeUse)
            << Use(1, 25, 5, Highlighting::TypeUse)
            << Use(3, 9, 1, Highlighting::TypeUse)
            << Use(3, 11, 8, Highlighting::FunctionDeclarationUse)
            << Use(6, 16, 1, Highlighting::TypeUse)
            << Use(6, 25, 8, Highlighting::TypeUse)
            << Use(8, 9, 1, Highlighting::TypeUse)
            << Use(8, 12, 8, Highlighting::FunctionDeclarationUse)
            << Use(11, 7, 3, Highlighting::TypeUse)
            << Use(12, 10, 3, Highlighting::FunctionDeclarationUse)
            << Use(15, 6, 1, Highlighting::FunctionDeclarationUse)
            << Use(17, 5, 5, Highlighting::TypeUse)
            << Use(17, 11, 8, Highlighting::TypeUse)
            << Use(17, 20, 3, Highlighting::TypeUse)
            << Use(17, 27, 4, Highlighting::LocalUse)
            << Use(18, 5, 4, Highlighting::LocalUse)
            << Use(18, 14, 3, Highlighting::FunctionUse)
            << Use(19, 5, 4, Highlighting::LocalUse)
            << Use(19, 14, 3, Highlighting::FunctionUse));

    // TODO: This is a good candidate for a performance test.
    QByteArray excessive =
             "template <class T>\n"
             "class Singleton\n"
             "{\n"
             "public:\n"
             "    static T& instance() {}\n"
             "};\n"
             "\n"
             "void bar() {}\n"
             "\n"
             "void foo()\n"
             "{\n"
            ;
    for (int i = 0; i < 250; ++i)
        excessive += "    Singleton<INIManager>::instance().bar();\n";
    excessive += "}\n";
    UseList excessiveUses;
    excessiveUses
            << Use(1, 17, 1, Highlighting::TypeUse)
            << Use(2, 7, 9, Highlighting::TypeUse)
            << Use(5, 12, 1, Highlighting::TypeUse)
            << Use(5, 15, 8, Highlighting::FunctionDeclarationUse)
            << Use(8, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(10, 6, 3, Highlighting::FunctionDeclarationUse);
    for (int i = 0; i < 250; ++i) {
        excessiveUses
                << Use(12 + i, 5, 9, Highlighting::TypeUse)
                << Use(12 + i, 28, 8, Highlighting::FunctionUse);
    }
    QTest::newRow("QTCREATORBUG8974_danglingPointer")
        << excessive
        << excessiveUses;

    QTest::newRow("operatorAsteriskOfNestedClassOfTemplateClass_QTCREATORBUG9006")
        << _("struct Foo { int foo; };\n"
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
             "}\n")
        << (UseList()
            << Use(1, 8, 3, Highlighting::TypeUse)
            << Use(1, 18, 3, Highlighting::FieldUse)
            << Use(3, 16, 1, Highlighting::TypeUse)
            << Use(4, 8, 5, Highlighting::TypeUse)
            << Use(6, 10, 6, Highlighting::TypeUse)
            << Use(8, 11, 1, Highlighting::TypeUse)
            << Use(8, 14, 8, Highlighting::FunctionDeclarationUse)
            << Use(8, 35, 1, Highlighting::FieldUse)
            << Use(9, 5, 1, Highlighting::TypeUse)
            << Use(9, 7, 1, Highlighting::FieldUse)
            << Use(13, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(15, 3, 5, Highlighting::TypeUse)
            << Use(15, 9, 3, Highlighting::TypeUse)
            << Use(15, 15, 6, Highlighting::TypeUse)
            << Use(15, 22, 6, Highlighting::LocalUse)
            << Use(16, 5, 6, Highlighting::LocalUse)
            << Use(16, 13, 3, Highlighting::FieldUse));

    QTest::newRow("templated_functions")
        << _("struct D {};\n"                      // line 1
             "struct A {\n"                        // line 2
             "    template<typename T> int B();\n" // line 3
             "    void C() {\n"                    // line 4
             "        B<D>();\n"                   // line 5
             "        this->B<D>();\n"             // line 6
             "    }\n"                             // line 7
             "};\n")                               // line 8
        << (UseList()
            << Use(1, 8, 1, Highlighting::TypeUse)
            << Use(2, 8, 1, Highlighting::TypeUse)
            << Use(3, 23, 1, Highlighting::TypeUse)
            << Use(3, 30, 1, Highlighting::FunctionDeclarationUse)
            << Use(4, 10, 1, Highlighting::FunctionDeclarationUse)
            << Use(5, 9, 1, Highlighting::FunctionUse)
            << Use(5, 11, 1, Highlighting::TypeUse)
            << Use(6, 15, 1, Highlighting::FunctionUse)
            << Use(6, 17, 1, Highlighting::TypeUse));

    QTest::newRow("AnonymousClass")
        << _("struct\n"
             "{\n"
             "  int foo;\n"
             "} Foo;\n"
             "\n"
             "void fun()\n"
             "{\n"
             "  foo = 3;\n"
             "}\n")
        << (UseList()
            << Use(3, 7, 3, Highlighting::FieldUse)
            << Use(6, 6, 3, Highlighting::FunctionDeclarationUse));

    QTest::newRow("QTCREATORBUG9098")
        << _("template <typename T>\n"
             "class B\n"
             "{\n"
             "public:\n"
             "    C<T> c;\n"
             "};\n"
             "template <typename T>\n"
             "class A\n"
             "{\n"
             "public:\n"
             "    B<T> b;\n"
             "    void fun()\n"
             "    {\n"
             "        b.c;\n"
             "    }\n"
             "};\n")
        << (UseList()
            << Use(1, 20, 1, Highlighting::TypeUse)
            << Use(2, 7, 1, Highlighting::TypeUse)
            << Use(5, 7, 1, Highlighting::TypeUse)
            << Use(5, 10, 1, Highlighting::FieldUse)
            << Use(7, 20, 1, Highlighting::TypeUse)
            << Use(8, 7, 1, Highlighting::TypeUse)
            << Use(11, 5, 1, Highlighting::TypeUse)
            << Use(11, 7, 1, Highlighting::TypeUse)
            << Use(11, 10, 1, Highlighting::FieldUse)
            << Use(12, 10, 3, Highlighting::FunctionDeclarationUse)
            << Use(14, 9, 1, Highlighting::FieldUse)
            << Use(14, 11, 1, Highlighting::FieldUse));

    QTest::newRow("AnonymousClass_insideNamespace")
        << _("struct { int foo1; } Foo1;\n"
             "void bar1()\n"
             "{\n"
             "  Foo1.foo1 = 42;\n"
             "}\n"
             "namespace Ns1 {\n"
             "  struct { int foo2; } Foo2;\n"
             "  void bar2()\n"
             "  {\n"
             "    Foo2.foo2 = 42;\n"
             "  }\n"
             "}\n"
             "namespace Ns2 {\n"
             "  struct {\n"
             "    struct { struct { int foo3; }; };\n"
             "    void func() { foo3 = 42; }\n"
             "  } Foo3;\n"
             "  void bar3()\n"
             "  {\n"
             "    Foo3.foo3 = 42;\n"
             "  }\n"
             "}\n")
        << (UseList()
            << Use(1, 14, 4, Highlighting::FieldUse)
            << Use(2, 6, 4, Highlighting::FunctionDeclarationUse)
            << Use(4, 8, 4, Highlighting::FieldUse)
            << Use(6, 11, 3, Highlighting::TypeUse)
            << Use(7, 16, 4, Highlighting::FieldUse)
            << Use(8, 8, 4, Highlighting::FunctionDeclarationUse)
            << Use(10, 10, 4, Highlighting::FieldUse)
            << Use(13, 11, 3, Highlighting::TypeUse)
            << Use(15, 27, 4, Highlighting::FieldUse)
            << Use(16, 10, 4, Highlighting::FunctionDeclarationUse)
            << Use(16, 19, 4, Highlighting::FieldUse)
            << Use(18, 8, 4, Highlighting::FunctionDeclarationUse)
            << Use(20, 10, 4, Highlighting::FieldUse));

    QTest::newRow("AnonymousClass_insideFunction")
        << _("int foo()\n"
              "{\n"
              "    union\n"
              "    {\n"
              "        int foo1;\n"
              "        int foo2;\n"
              "    };\n"
              "}\n")
        << (UseList()
            << Use(1, 5, 3, Highlighting::FunctionDeclarationUse)
            << Use(5, 13, 4, Highlighting::FieldUse)
            << Use(6, 13, 4, Highlighting::FieldUse));

    QTest::newRow("AnonymousClass_QTCREATORBUG8963")
        << _("typedef enum {\n"
              "    FIRST\n"
              "} isNotInt;\n"
              "typedef struct {\n"
              "    int isint;\n"
              "    int isNotInt;\n"
              "} Struct;\n"
              "void foo()\n"
              "{\n"
              "    Struct s;\n"
              "    s.isint;\n"
              "    s.isNotInt;\n"
              "    FIRST;\n"
              "}\n")
        << (UseList()
            << Use(2, 5, 5, Highlighting::EnumerationUse)
            << Use(3, 3, 8, Highlighting::TypeUse)
            << Use(5, 9, 5, Highlighting::FieldUse)
            << Use(6, 9, 8, Highlighting::FieldUse)
            << Use(7, 3, 6, Highlighting::TypeUse)
            << Use(8, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(10, 5, 6, Highlighting::TypeUse)
            << Use(10, 12, 1, Highlighting::LocalUse)
            << Use(11, 5, 1, Highlighting::LocalUse)
            << Use(11, 7, 5, Highlighting::FieldUse)
            << Use(12, 5, 1, Highlighting::LocalUse)
            << Use(12, 7, 8, Highlighting::FieldUse)
            << Use(13, 5, 5, Highlighting::EnumerationUse));

    QTest::newRow("class_declaration_with_object_name_nested_in_function")
        << _("int foo()\n"
              "{\n"
              "    struct Nested\n"
              "    {\n"
              "        int i;\n"
              "    } n;\n"
              "    n.i = 42;\n"
              "}\n")
        << (UseList()
            << Use(1, 5, 3, Highlighting::FunctionDeclarationUse)
            << Use(3, 12, 6, Highlighting::TypeUse)
            << Use(5, 13, 1, Highlighting::FieldUse)
            << Use(6, 7, 1, Highlighting::LocalUse)
            << Use(7, 5, 1, Highlighting::LocalUse)
            << Use(7, 7, 1, Highlighting::FieldUse));

    QTest::newRow("highlightingTypeWhenUsingNamespaceClass_QTCREATORBUG7903_globalNamespace")
        << _("namespace NS {\n"
             "class Foo {};\n"
             "}\n"
             "using NS::Foo;\n"
             "void fun()\n"
             "{\n"
             "    Foo foo;\n"
             "}\n")
        << (UseList()
            << Use(1, 11, 2, Highlighting::TypeUse)
            << Use(2, 7, 3, Highlighting::TypeUse)
            << Use(4, 7, 2, Highlighting::TypeUse)
            << Use(4, 11, 3, Highlighting::TypeUse)
            << Use(5, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(7, 5, 3, Highlighting::TypeUse)
            << Use(7, 9, 3, Highlighting::LocalUse));

    QTest::newRow("highlightingTypeWhenUsingNamespaceClass_QTCREATORBUG7903_namespace")
        << _("namespace NS {\n"
              "class Foo {};\n"
              "}\n"
              "namespace NS1 {\n"
              "using NS::Foo;\n"
              "void fun()\n"
              "{\n"
              "    Foo foo;\n"
              "}\n"
              "}\n")
        << (UseList()
            << Use(1, 11, 2, Highlighting::TypeUse)
            << Use(2, 7, 3, Highlighting::TypeUse)
            << Use(4, 11, 3, Highlighting::TypeUse)
            << Use(5, 7, 2, Highlighting::TypeUse)
            << Use(5, 11, 3, Highlighting::TypeUse)
            << Use(6, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(8, 5, 3, Highlighting::TypeUse)
            << Use(8, 9, 3, Highlighting::LocalUse));

    QTest::newRow("highlightingTypeWhenUsingNamespaceClass_QTCREATORBUG7903_insideFunction")
        << _("namespace NS {\n"
             "class Foo {};\n"
             "}\n"
             "void fun()\n"
             "{\n"
             "    using NS::Foo;\n"
             "    Foo foo;\n"
             "}\n")
        << (UseList()
            << Use(1, 11, 2, Highlighting::TypeUse)
            << Use(2, 7, 3, Highlighting::TypeUse)
            << Use(4, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(6, 11, 2, Highlighting::TypeUse)
            << Use(6, 15, 3, Highlighting::TypeUse)
            << Use(7, 5, 3, Highlighting::TypeUse)
            << Use(7, 9, 3, Highlighting::LocalUse));

    QTest::newRow("crashWhenUsingNamespaceClass_QTCREATORBUG9323_globalNamespace")
        << _("namespace NS {\n"
             "class Foo {};\n"
             "}\n"
             "using ::;\n"
             "void fun()\n"
             "{\n"
             "    Foo foo;\n"
             "}\n")
        << (UseList()
            << Use(1, 11, 2, Highlighting::TypeUse)
            << Use(2, 7, 3, Highlighting::TypeUse)
            << Use(5, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(7, 9, 3, Highlighting::LocalUse));

    QTest::newRow("crashWhenUsingNamespaceClass_QTCREATORBUG9323_namespace")
        << _("namespace NS {\n"
             "class Foo {};\n"
             "}\n"
             "namespace NS1 {\n"
             "using ::;\n"
             "void fun()\n"
             "{\n"
             "    Foo foo;\n"
             "}\n"
             "}\n")
        << (UseList()
            << Use(1, 11, 2, Highlighting::TypeUse)
            << Use(2, 7, 3, Highlighting::TypeUse)
            << Use(4, 11, 3, Highlighting::TypeUse)
            << Use(6, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(8, 9, 3, Highlighting::LocalUse));

    QTest::newRow("crashWhenUsingNamespaceClass_QTCREATORBUG9323_insideFunction")
        << _("namespace NS {\n"
             "class Foo {};\n"
             "}\n"
             "void fun()\n"
             "{\n"
             "    using ::;\n"
             "    Foo foo;\n"
             "}\n")
        << (UseList()
            << Use(1, 11, 2, Highlighting::TypeUse)
            << Use(2, 7, 3, Highlighting::TypeUse)
            << Use(4, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(7, 9, 3, Highlighting::LocalUse));

    QTest::newRow("alias_decl_QTCREATORBUG9386")
        << _("using wobble = int;\n"
             "wobble cobble = 1;\n")
        << (UseList()
            << Use(1, 7, 6, Highlighting::TypeUse)
            << Use(2, 1, 6, Highlighting::TypeUse));

    QTest::newRow("enum_inside_block_inside_function_QTCREATORBUG5456")
        << _("void foo()\n"
             "{\n"
             "   {\n"
             "       enum E { e1, e2, e3 };\n"
             "       E e = e1;\n"
             "   }\n"
             "}\n")
        << (UseList()
            << Use(1, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(4, 13, 1, Highlighting::TypeUse)
            << Use(4, 17, 2, Highlighting::EnumerationUse)
            << Use(4, 21, 2, Highlighting::EnumerationUse)
            << Use(4, 25, 2, Highlighting::EnumerationUse)
            << Use(5, 8, 1, Highlighting::TypeUse)
            << Use(5, 10, 1, Highlighting::LocalUse)
            << Use(5, 14, 2, Highlighting::EnumerationUse));

    QTest::newRow("enum_inside_function_QTCREATORBUG5456")
        << _("void foo()\n"
             "{\n"
             "   enum E { e1, e2, e3 };\n"
             "   E e = e1;\n"
             "}\n")
        << (UseList()
            << Use(1, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(3, 9, 1, Highlighting::TypeUse)
            << Use(3, 13, 2, Highlighting::EnumerationUse)
            << Use(3, 17, 2, Highlighting::EnumerationUse)
            << Use(3, 21, 2, Highlighting::EnumerationUse)
            << Use(4, 4, 1, Highlighting::TypeUse)
            << Use(4, 6, 1, Highlighting::LocalUse)
            << Use(4, 10, 2, Highlighting::EnumerationUse));

    QTest::newRow("using_inside_different_namespace_QTCREATORBUG7978")
        << _("struct S {};\n"
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
             "}\n")
        << (UseList()
            << Use(1, 8, 1, Highlighting::TypeUse)
            << Use(2, 11, 3, Highlighting::TypeUse)
            << Use(4, 24, 1, Highlighting::TypeUse)
            << Use(4, 34, 10, Highlighting::TypeUse)
            << Use(6, 11, 2, Highlighting::TypeUse)
            << Use(8, 11, 3, Highlighting::TypeUse)
            << Use(8, 16, 10, Highlighting::TypeUse)
            << Use(10, 6, 3, Highlighting::FunctionDeclarationUse)
            << Use(12, 5, 2, Highlighting::TypeUse)
            << Use(12, 9, 10, Highlighting::TypeUse)
            << Use(12, 20, 1, Highlighting::TypeUse)
            << Use(12, 23, 1, Highlighting::LocalUse));

    QTest::newRow("using_inside_different_block_of_scope_unnamed_namespace_QTCREATORBUG12357")
            << _("namespace \n"
                 "{\n"
                 "    namespace Ns { struct Foo {}; }\n"
                 "    using Ns::Foo;\n"
                 "}\n"
                 "void fun()\n"
                 "{\n"
                 "    Foo foo;\n"
                 "}\n")
            << (QList<Use>()
                << Use(3, 15, 2, Highlighting::TypeUse)
                << Use(3, 27, 3, Highlighting::TypeUse)
                << Use(4, 11, 2, Highlighting::TypeUse)
                << Use(4, 15, 3, Highlighting::TypeUse)
                << Use(6, 6, 3, Highlighting::FunctionDeclarationUse)
                << Use(8, 5, 3, Highlighting::TypeUse)
                << Use(8, 9, 3, Highlighting::LocalUse)
                );

    QTest::newRow("using_inside_different_block_of_scope_named_namespace_QTCREATORBUG12357")
            << _("namespace NS1\n"
                 "{\n"
                 "    namespace Ns { struct Foo {}; }\n"
                 "    using Ns::Foo;\n"
                 "}\n"
                 "namespace NS1\n"
                 "{\n"
                 "    void fun()\n"
                 "    {\n"
                 "        Foo foo;\n"
                 "    }\n"
                 "}\n"
                 )
            << (QList<Use>()
                << Use(1, 11, 3, Highlighting::TypeUse)
                << Use(3, 15, 2, Highlighting::TypeUse)
                << Use(3, 27, 3, Highlighting::TypeUse)
                << Use(4, 11, 2, Highlighting::TypeUse)
                << Use(4, 15, 3, Highlighting::TypeUse)
                << Use(6, 11, 3, Highlighting::TypeUse)
                << Use(8, 10, 3, Highlighting::FunctionDeclarationUse)
                << Use(10, 13, 3, Highlighting::LocalUse)
                );

    QTest::newRow("template_alias")
            << _("template<class T>\n"
                 "using Foo = Bar<T>;\n")
            << (QList<Use>()
                << Use(1, 16, 1, Highlighting::TypeUse)
                << Use(2, 7, 3, Highlighting::TypeUse)
                << Use(2, 17, 1, Highlighting::TypeUse)
                );

    QTest::newRow("using_inside_different_namespace_QTCREATORBUG7978")
        << _("class My" TEST_UNICODE_IDENTIFIER "Type { int " TEST_UNICODE_IDENTIFIER "Member; };\n"
             "void f(My" TEST_UNICODE_IDENTIFIER "Type var" TEST_UNICODE_IDENTIFIER ")\n"
             "{ var" TEST_UNICODE_IDENTIFIER "." TEST_UNICODE_IDENTIFIER "Member = 0; }\n")
        << (UseList()
            << Use(1, 7, 10, Highlighting::TypeUse)
            << Use(1, 24, 10, Highlighting::FieldUse)
            << Use(2, 6, 1, Highlighting::FunctionDeclarationUse)
            << Use(2, 8, 10, Highlighting::TypeUse)
            << Use(2, 19, 7, Highlighting::LocalUse)
            << Use(3, 3, 7, Highlighting::LocalUse)
            << Use(3, 11, 10, Highlighting::FieldUse));

    QTest::newRow("instantation_as_function_call_QTCREATORBUG15212")
        << _("struct Foo {};\n"
             "template <typename Type> struct test {\n"
             "  test() {}\n"
             "  test(int, int) {}\n"
             "};\n"
             "void test(int int_argument) {\n"
             "  const int very_long_constant_of_type_int = 11111111111111111;\n"
             "  test<Foo> foo1;\n"
             "  test<Foo> foo2(int_argument, int_argument);\n"
             "  test<Foo> foo3(very_long_constant_of_type_int,\n"
             "                 very_long_constant_of_type_int);\n"
             "  test<int> size1(int_argument, int_argument);\n"
             "  (void)foo1, foo2, foo3, size1;\n"
             "  test(int_argument);\n"
             "}\n")
        << (UseList()
            << Use(1, 8, 3, Highlighting::TypeUse)
            << Use(2, 20, 4, Highlighting::TypeUse)
            << Use(2, 33, 4, Highlighting::TypeUse)
            << Use(3, 3, 4, Highlighting::TypeUse)
            << Use(4, 3, 4, Highlighting::TypeUse)
            << Use(6, 6, 4, Highlighting::FunctionDeclarationUse)
            << Use(6, 15, 12, Highlighting::LocalUse)
            << Use(7, 13, 30, Highlighting::LocalUse)
            << Use(8, 3, 4, Highlighting::TypeUse)
            << Use(8, 8, 3, Highlighting::TypeUse)
            << Use(8, 13, 4, Highlighting::LocalUse)
            << Use(9, 3, 4, Highlighting::TypeUse)
            << Use(9, 8, 3, Highlighting::TypeUse)
            << Use(9, 13, 4, Highlighting::LocalUse)
            << Use(9, 18, 12, Highlighting::LocalUse)
            << Use(9, 32, 12, Highlighting::LocalUse)
            << Use(10, 3, 4, Highlighting::TypeUse)
            << Use(10, 8, 3, Highlighting::TypeUse)
            << Use(10, 13, 4, Highlighting::LocalUse)
            << Use(10, 18, 30, Highlighting::LocalUse)
            << Use(11, 18, 30, Highlighting::LocalUse)
            << Use(12, 3, 4, Highlighting::TypeUse)
            << Use(12, 13, 5, Highlighting::LocalUse)
            << Use(12, 19, 12, Highlighting::LocalUse)
            << Use(12, 33, 12, Highlighting::LocalUse)
            << Use(13, 9, 4, Highlighting::LocalUse)
            << Use(13, 15, 4, Highlighting::LocalUse)
            << Use(13, 21, 4, Highlighting::LocalUse)
            << Use(13, 27, 5, Highlighting::LocalUse)
            << Use(14, 3, 4, Highlighting::FunctionUse)
            << Use(14, 8, 12, Highlighting::LocalUse));

    QTest::newRow("unicodeIdentifier1")
        << _("class My" TEST_UNICODE_IDENTIFIER "Type { int " TEST_UNICODE_IDENTIFIER "Member; };\n"
             "void f(My" TEST_UNICODE_IDENTIFIER "Type var" TEST_UNICODE_IDENTIFIER ")\n"
             "{ var" TEST_UNICODE_IDENTIFIER "." TEST_UNICODE_IDENTIFIER "Member = 0; }\n")
        << (UseList()
            << Use(1, 7, 10, Highlighting::TypeUse)
            << Use(1, 24, 10, Highlighting::FieldUse)
            << Use(2, 6, 1, Highlighting::FunctionDeclarationUse)
            << Use(2, 8, 10, Highlighting::TypeUse)
            << Use(2, 19, 7, Highlighting::LocalUse)
            << Use(3, 3, 7, Highlighting::LocalUse)
            << Use(3, 11, 10, Highlighting::FieldUse));

    QTest::newRow("unicodeIdentifier2")
        << _("class v" TEST_UNICODE_IDENTIFIER "\n"
             "{\n"
             "public:\n"
             "    v" TEST_UNICODE_IDENTIFIER "();\n"
             "    ~v" TEST_UNICODE_IDENTIFIER "();\n"
             "};\n"
             "\n"
             "v" TEST_UNICODE_IDENTIFIER "::v" TEST_UNICODE_IDENTIFIER "() {}\n"
             "v" TEST_UNICODE_IDENTIFIER "::~v" TEST_UNICODE_IDENTIFIER "() {}\n")
        << (UseList()
            << Use(1, 7, 5, Highlighting::TypeUse)
            << Use(4, 5, 5, Highlighting::TypeUse)
            << Use(5, 6, 5, Highlighting::TypeUse)
            << Use(5, 6, 5, Highlighting::TypeUse)
            << Use(8, 1, 5, Highlighting::TypeUse)
            << Use(8, 8, 5, Highlighting::FunctionDeclarationUse)
            << Use(9, 1, 5, Highlighting::TypeUse)
            << Use(9, 1, 5, Highlighting::TypeUse)
            << Use(9, 9, 5, Highlighting::TypeUse));

#define UC_U10302_4TIMES UC_U10302 UC_U10302 UC_U10302 UC_U10302
#define UC_U10302_12TIMES UC_U10302_4TIMES UC_U10302_4TIMES UC_U10302_4TIMES
    QTest::newRow("unicodeComments1")
        << _("#define NULL 0\n"
             "\n"
             "// " UC_U10302_12TIMES "\n"
             "// " UC_U10302_12TIMES "\n"
             "\n"
             "class Foo {\n"
                 "double f(bool b = NULL);\n"
                 "Foo *x;\n"
             "};\n")
        << (UseList()
            << Use(6, 7, 3, Highlighting::TypeUse)
            << Use(7, 8, 1, Highlighting::FunctionDeclarationUse)
            << Use(8, 1, 3, Highlighting::TypeUse)
            << Use(8, 6, 1, Highlighting::FieldUse));
#undef UC_U10302_12TIMES
#undef UC_U10302_4TIMES
}

void tst_CheckSymbols::test_checksymbols_macroUses()
{
    QFETCH(QByteArray, source);
    QFETCH(UseList, expectedUsesAll);
    QFETCH(UseList, expectedUsesMacros);

    TestCase(source, expectedUsesAll, expectedUsesMacros);
}

void tst_CheckSymbols::test_checksymbols_macroUses_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<UseList>("expectedUsesAll");
    QTest::addColumn<UseList>("expectedUsesMacros");

    QTest::newRow("MacroUse")
        << _("#define FOO 1+1\n"
             "int f() { FOO; }\n")
        << (UseList()
            << Use(1, 9, 3, Highlighting::MacroUse)
            << Use(2, 5, 1, Highlighting::FunctionDeclarationUse)
            << Use(2, 11, 3, Highlighting::MacroUse))
        << (UseList()
            << Use(1, 9, 3, Highlighting::MacroUse)
            << Use(2, 11, 3, Highlighting::MacroUse));
}

void tst_CheckSymbols::test_checksymbols_infiniteLoop()
{
    QFETCH(QByteArray, source1);
    QFETCH(QByteArray, source2);

    const QString filePath1 = QDir::tempPath() + QLatin1String("/file1.h");
    Tests::TestCase::writeFile(filePath1, source1);

    const QString filePath2 = QDir::tempPath() + QLatin1String("/file2.h");
    Tests::TestCase::writeFile(filePath2, source2);

    const Document::Ptr document1 = TestCase::createDocument(filePath1, source1);
    document1->addIncludeFile(Document::Include("file2.h", filePath2, 1, Client::IncludeLocal));
    Snapshot snapshot;
    snapshot.insert(document1);
    snapshot.insert(TestCase::createDocument(filePath2, source2));

    TestCase::runCheckSymbols(document1, snapshot);
}

void tst_CheckSymbols::test_checkForValidSymbolFileId()
{
    const QByteArray contents =
        "constexpr int parent_of(const int f) { return 1; }\n"
        "\n"
        "template <typename T> struct wrapper { const T* ptr; };\n"
        "template <int> struct Dummy;\n"
        "\n"
        "namespace impl {\n"
        "    template <int f>\n"
        "    struct dummy_impl {\n"
        "        wrapper<Dummy<parent_of(f)>> parent;\n"
        "    };\n"
        "}\n"
        "\n"
        "template <int f>\n"
        "struct Dummy : impl::dummy_impl<f> {};\n"
        "\n"
        "void client()\n"
        "{\n"
        "    wrapper<Dummy<1>> a;\n"
        "    a.ptr->parent.ptr;\n"
        "}\n";

    BaseTestCase tc(contents);
}

void tst_CheckSymbols::test_parentOfBlock()
{
    const QByteArray source = "void C::f()\n"
                              "{\n"
                              "    enum E { e1 };\n"
                              "}\n";
    BaseTestCase tc(source);
}

void tst_CheckSymbols::test_infiniteLoop()
{
    const QByteArray source =
        "template <class> struct TNode;\n"
        "template <class> struct TMetaNode;\n"
        "\n"
        "template <class X>\n"
        "struct TTraits {\n"
        "   using TX        = X;\n"
        "   using TNodeType = TNode<TX>;\n"
        "};\n"
        "\n"
        "template <class X>\n"
        "struct TMetaNode {\n"
        "   using TTraitsType = TTraits<X>;\n"
        "};\n"
        "\n"
        "template <class X>\n"
        "void nonmember() {\n"
        "   using TMetaNodeType = TMetaNode<X>;\n"
        "}\n"
        "\n"
        "template <class X>\n"
        "struct TNode {\n"
        "   using TTraitsType = TTraits<X>;\n"
        "   void member();\n"
        "};\n"
        "\n"
        "template <class X>\n"
        "void TNode<X>::member() {}\n"
        ;

    BaseTestCase tc(source);
}

void tst_CheckSymbols::test_checksymbols_infiniteLoop_data()
{
    QTest::addColumn<QByteArray>("source1");
    QTest::addColumn<QByteArray>("source2");

    QTest::newRow("1")
        <<
         _("#include \"file2.h\"\n"
           "\n"
           "template<class _Elem, class _Traits>\n"
           "class basic_ios {\n"
           "        typedef basic_ostream<_Elem, _Traits> _Myos;\n"
           "};\n"
           "\n"
           "template<class _Elem, class _Traits>\n"
           "class basic_ostream {\n"
           "        typedef basic_ostream<_Elem, _Traits> _Myt;\n"
           "        typedef ostreambuf_iterator<_Elem, _Traits> _Iter;\n"
           "};\n")
        <<
         _("template<class _Elem, class _Traits>\n"
           "class basic_streambuf {\n"
           "        typedef basic_streambuf<_Elem, _Traits> _Myt;\n"
           "};\n"
           "\n"
           "template<class _Elem, class _Traits>\n"
           "class ostreambuf_iterator {\n"
           "    typedef _Traits traits_type;\n"
           "        typedef basic_streambuf<_Elem, _Traits> streambuf_type;\n"
           "        typedef basic_ostream<_Elem, _Traits> ostream_type;\n"
           "};\n")
           ;

    QTest::newRow("2")
        <<
         _("#include \"file2.h\"\n"
           "\n"
           "template<class _Ty >\n"
           "struct _List_base_types\n"
           "{\n"
           "        typedef typename _Wrap_alloc<_Alloc>::template rebind<_Ty>::other _Alty;\n"
           "        typedef typename _Alty::template rebind<_Node>::other _Alnod_type;\n"
           "};\n"
           "\n"
           "template<class _Alloc_types>\n"
           "struct _List_alloc \n"
           "{\n"
           "        const _Alloc_types::_Alnod_type& _Getal() const {}\n"
           "};\n"
           "\n"
           "template<class _Ty, class _Alloc>\n"
           "struct _List_buy : public _List_alloc< _List_base_types<_Ty> >\n"
           "{\n"
           "        void foo()\n"
           "        {\n"
           "                this->_Getal().construct(1, 2);\n"
           "                this->_Getal().deallocate(0, 1);\n"
           "        }\n"
           "};\n")
        <<
         _("template<class _Alloc>\n"
           "struct _Wrap_alloc : public _Alloc\n"
           "{\n"
           "    typedef _Alloc _Mybase;\n"
           "    template<class _Other> struct rebind { typedef _Wrap_alloc<_Other_alloc> other; };\n"
           "\n"
           "    void deallocate(pointer _Ptr, size_type _Count) {}\n"
           "    void construct(value_type *_Ptr) {}\n"
           "};\n")
           ;
}

void tst_CheckSymbols::findField()
{
    QFETCH(QByteArray, source);

    int position = source.indexOf('@');
    QVERIFY(position != -1);
    QByteArray truncated = source;
    truncated.truncate(position);
    const unsigned line = truncated.count('\n') + 1;
    const unsigned column = position - truncated.lastIndexOf('\n', position) + 1;
    source[position] = ' ';
    BaseTestCase tc(source);
    Use use = tc.findUse(line, column);
    QEXPECT_FAIL("pointer_indirect_specialization", "QTCREATORBUG-14141", Abort);
    QEXPECT_FAIL("pointer_indirect_specialization_typedef", "QTCREATORBUG-14141", Abort);
    QEXPECT_FAIL("pointer_indirect_specialization_double_indirection", "QTCREATORBUG-14141", Abort);
    QEXPECT_FAIL("instantiation_of_pointer_typedef_in_block", "QTCREATORBUG-14141", Abort);
    QEXPECT_FAIL("pointer_indirect_specialization_double_indirection_with_base", "QTCREATORBUG-14141", Abort);
    QEXPECT_FAIL("recursive_instantiation_of_template_type", "QTCREATORBUG-14237", Abort);
    QEXPECT_FAIL("recursive_instantiation_of_template_type_2", "QTCREATORBUG-14141", Abort);
    QVERIFY(use.isValid());
    QVERIFY(use.kind == Highlighting::FieldUse);
}

void tst_CheckSymbols::findField_data()
{
    QTest::addColumn<QByteArray>("source");

    QTest::newRow("pointer_indirect_specialization") << _(
        "template<typename T>\n"
        "struct Traits { typedef typename T::pointer pointer; };\n"
        "\n"
        "template<typename _Tp>\n"
        "struct Traits<_Tp*> { typedef _Tp *pointer; };\n"
        "\n"
        "template<typename T>\n"
        "class Temp\n"
        "{\n"
        "protected:\n"
        "   typedef Traits<T> TraitsT;\n"
        "\n"
        "public:\n"
        "   typedef typename TraitsT::pointer pointer;\n"
        "   pointer p;\n"
        "};\n"
        "\n"
        "struct Foo { int bar; };\n"
        "\n"
        "void func()\n"
        "{\n"
        "   Temp<Foo *> t;\n"
        "   t.p->@bar;\n"
        "}\n"
    );

    QTest::newRow("pointer_indirect_specialization_typedef") << _(
        "template<typename T>\n"
        "struct Traits { typedef typename T::pointer pointer; };\n"
        "\n"
        "template<typename _Tp>\n"
        "struct Traits<_Tp*> { typedef _Tp *pointer; };\n"
        "\n"
        "struct Foo { int bar; };\n"
        "\n"
        "class Temp\n"
        "{\n"
        "protected:\n"
        "   typedef Foo *FooPtr;\n"
        "   typedef Traits<FooPtr> TraitsT;\n"
        "\n"
        "public:\n"
        "   typedef typename TraitsT::pointer pointer;\n"
        "   pointer p;\n"
        "};\n"
        "\n"
        "void func()\n"
        "{\n"
        "   Temp t;\n"
        "   t.p->@bar;\n"
        "}\n"
    );

    QTest::newRow("instantiation_of_pointer_typedef_in_block") << _(
        "template<typename _Tp>\n"
        "struct Temp { _Tp p; };\n"
        "\n"
        "struct Foo { int bar; };\n"
        "\n"
        "void func()\n"
        "{\n"
        "   typedef Foo *pointer;\n"
        "   Temp<pointer> t;\n"
        "   t.p->@bar;\n"
        "}\n"
    );

    QTest::newRow("pointer_indirect_specialization_double_indirection") << _(
        "template<typename _Tp>\n"
        "struct Traits { };\n"
        "\n"
        "template<typename _Tp>\n"
        "struct Traits<_Tp*> { typedef _Tp *pointer; };\n"
        "\n"
        "struct Foo { int bar; };\n"
        "\n"
        "template<typename _Tp>\n"
        "struct IndirectT\n"
        "{\n"
        "   typedef Traits<_Tp> TraitsT;\n"
        "   typedef typename TraitsT::pointer pointer;\n"
        "   pointer p;\n"
        "};\n"
        "\n"
        "template<typename _Tp>\n"
        "struct Temp\n"
        "{\n"
        "   typedef _Tp *pointer;\n"
        "   typedef IndirectT<pointer> indirect;\n"
        "};\n"
        "\n"
        "void func()\n"
        "{\n"
        "   Temp<Foo>::indirect t;\n"
        "   t.p->@bar;\n"
        "}\n"
    );

    QTest::newRow("pointer_indirect_specialization_double_indirection_with_base") << _(
        "template<typename _Tp>\n"
        "struct Traits { };\n"
        "\n"
        "template<typename _Tp>\n"
        "struct Traits<_Tp*> { typedef _Tp *pointer; };\n"
        "\n"
        "struct Foo { int bar; };\n"
        "\n"
        "template<typename _Tp>\n"
        "struct IndirectT\n"
        "{\n"
        "   typedef Traits<_Tp> TraitsT;\n"
        "   typedef typename TraitsT::pointer pointer;\n"
        "   pointer p;\n"
        "};\n"
        "\n"
        "template<typename _Tp>\n"
        "struct TempBase { typedef _Tp *pointer; };\n"
        "\n"
        "template<typename _Tp>\n"
        "struct Temp : public TempBase<_Tp>\n"
        "{\n"
        "   typedef TempBase<_Tp> _Base;\n"
        "   typedef typename _Base::pointer pointer;\n"
        "   typedef IndirectT<pointer> indirect;\n"
        "};\n"
        "\n"
        "void func()\n"
        "{\n"
        "   Temp<Foo>::indirect t;\n"
        "   t.p->@bar;\n"
        "}\n"
    );

    QTest::newRow("recursive_instantiation_of_template_type") << _(
        "template<typename _Tp>\n"
        "struct Temp { typedef _Tp value_type; };\n"
        "\n"
        "struct Foo { int bar; };\n"
        "\n"
        "void func()\n"
        "{\n"
        "   Temp<Temp<Foo> >::value_type::value_type *p;\n"
        "   p->@bar;\n"
        "}\n"
    );

    QTest::newRow("recursive_instantiation_of_template_type_2") << _(
        "template<typename _Tp>\n"
        "struct Temp { typedef _Tp value_type; };\n"
        "\n"
        "struct Foo { int bar; };\n"
        "\n"
        "void func()\n"
        "{\n"
        "   Temp<Temp<Foo>::value_type>::value_type *p;\n"
        "   p->@bar;\n"
        "}\n"
    );
}

QTEST_APPLESS_MAIN(tst_CheckSymbols)
#include "tst_checksymbols.moc"
