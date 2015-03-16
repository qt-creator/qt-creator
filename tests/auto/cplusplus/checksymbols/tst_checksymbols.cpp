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

using namespace CPlusPlus;
using namespace CppTools;

typedef QByteArray _;
typedef CheckSymbols::Result Use;
typedef CheckSymbols::Kind UseKind;
typedef SemanticHighlighter Highlighting;
typedef QList<Use> UseList;
Q_DECLARE_METATYPE(UseList)

static QString useKindToString(UseKind useKind)
{
    switch (useKind) {
    case Highlighting::Unknown:
        return QLatin1String("SemanticHighlighter::Unknown");
    case Highlighting::TypeUse:
        return QLatin1String("SemanticHighlighter::TypeUse");
    case Highlighting::LocalUse:
        return QLatin1String("SemanticHighlighter::LocalUse");
    case Highlighting::FieldUse:
        return QLatin1String("SemanticHighlighter::FieldUse");
    case Highlighting::EnumerationUse:
        return QLatin1String("SemanticHighlighter::EnumerationUse");
    case Highlighting::VirtualMethodUse:
        return QLatin1String("SemanticHighlighter::VirtualMethodUse");
    case Highlighting::LabelUse:
        return QLatin1String("SemanticHighlighter::LabelUse");
    case Highlighting::MacroUse:
        return QLatin1String("SemanticHighlighter::MacroUse");
    case Highlighting::FunctionUse:
        return QLatin1String("SemanticHighlighter::FunctionUse");
    case Highlighting::PseudoKeywordUse:
        return QLatin1String("SemanticHighlighter::PseudoKeywordUse");
    default:
        QTest::qFail("Unknown UseKind", __FILE__, __LINE__);
        return QLatin1String("Unknown UseKind");
    }
}

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

class TestCase
{
public:
    TestCase(const QByteArray &source,
             const UseList &expectedUsesAll,
             const UseList &expectedUsesMacros = UseList())
    {
        // Write source to temprorary file
        const QString filePath = QDir::tempPath() + QLatin1String("/file.h");
        Tests::TestCase::writeFile(filePath, source);

        // Processs source
        const Document::Ptr document = createDocument(filePath, source);
        Snapshot snapshot;
        snapshot.insert(document);

        // Collect symbols
        CheckSymbols::Future future = runCheckSymbols(document, snapshot, expectedUsesMacros);

        const int resultCount = future.resultCount();
        UseList actualUses;
        for (int i = 0; i < resultCount; ++i) {
            const Use use = future.resultAt(i);
            // When adding tests, you may want to uncomment the
            // following line in order to print out all found uses.
            // qDebug() << QTest::toString(use);
            actualUses.append(use);
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

    void test_parentOfBlock();
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
            << Use(1, 5, 1, Highlighting::FunctionUse)
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
            << Use(5, 5, 1, Highlighting::FunctionUse)
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

    QTest::newRow("VirtualMethodUse")
        << _("class B {\n"                   // 1
             "    virtual bool isThere();\n" // 2
             "};\n"                          // 3
             "class D: public B {\n"         // 4
             "    bool isThere();\n"         // 5
             "};\n")
        << (UseList()
            << Use(1, 7, 1, Highlighting::TypeUse)              // B
            << Use(2, 18, 7, Highlighting::VirtualMethodUse)    // isThere
            << Use(4, 7, 1, Highlighting::TypeUse)              // D
            << Use(4, 17, 1, Highlighting::TypeUse)             // B
            << Use(5, 10, 7, Highlighting::VirtualMethodUse));  // isThere

    QTest::newRow("LabelUse")
        << _("int f()\n"
             "{\n"
             "   goto g;\n"
             "   g: return 1;\n"
             "}\n")
        << (UseList()
            << Use(1, 5, 1, Highlighting::FunctionUse)
            << Use(3, 9, 1, Highlighting::LabelUse)
            << Use(4, 4, 1, Highlighting::LabelUse));

    QTest::newRow("FunctionUse")
        << _("int f();\n"
             "int g() { f(); }\n")
        << (UseList()
            << Use(1, 5, 1, Highlighting::FunctionUse)
            << Use(2, 5, 1, Highlighting::FunctionUse)
            << Use(2, 11, 1, Highlighting::FunctionUse));

    QTest::newRow("PseudoKeywordUse")
        << _("class D : public B {"
             "   virtual void f() override {}\n"
             "   virtual void f() final {}\n"
             "};\n")
        << (UseList()
            << Use(1, 7, 1, Highlighting::TypeUse)
            << Use(1, 37, 1, Highlighting::VirtualMethodUse)
            << Use(1, 41, 8, Highlighting::PseudoKeywordUse)
            << Use(2, 17, 1, Highlighting::VirtualMethodUse)
            << Use(2, 21, 5, Highlighting::PseudoKeywordUse));

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
            << Use(7, 14, 3, Highlighting::FunctionUse)
            << Use(11, 5, 5, Highlighting::TypeUse)
            << Use(11, 12, 3, Highlighting::FieldUse)
            << Use(13, 6, 5, Highlighting::TypeUse)
            << Use(13, 13, 5, Highlighting::TypeUse)
            << Use(13, 20, 3, Highlighting::FunctionUse)
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
            << Use(9, 6, 3, Highlighting::FunctionUse)
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
            << Use(3, 16, 3, Highlighting::FunctionUse)
            << Use(6, 6, 3, Highlighting::FunctionUse)
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
            << Use(3, 16, 3, Highlighting::FunctionUse)
            << Use(6, 6, 3, Highlighting::FunctionUse)
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
            << Use(3, 16, 3, Highlighting::FunctionUse)
            << Use(6, 17, 3, Highlighting::TypeUse)
            << Use(7, 6, 3, Highlighting::FunctionUse)
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
            << Use(3, 16, 3, Highlighting::FunctionUse)
            << Use(6, 8, 3, Highlighting::TypeUse)
            << Use(7, 6, 3, Highlighting::FunctionUse)
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
            << Use(3, 11, 8, Highlighting::FunctionUse)
            << Use(6, 16, 1, Highlighting::TypeUse)
            << Use(6, 25, 8, Highlighting::TypeUse)
            << Use(8, 9, 1, Highlighting::TypeUse)
            << Use(8, 12, 8, Highlighting::FunctionUse)
            << Use(11, 7, 3, Highlighting::TypeUse)
            << Use(12, 10, 3, Highlighting::FunctionUse)
            << Use(15, 6, 1, Highlighting::FunctionUse)
            << Use(17, 5, 5, Highlighting::TypeUse)
            << Use(17, 11, 8, Highlighting::TypeUse)
            << Use(17, 20, 3, Highlighting::TypeUse)
            << Use(17, 27, 4, Highlighting::LocalUse)
            << Use(18, 5, 4, Highlighting::LocalUse)
            << Use(18, 14, 3, Highlighting::FunctionUse)
            << Use(19, 5, 4, Highlighting::LocalUse)
            << Use(19, 14, 3, Highlighting::FunctionUse));

    // TODO: This is a good candidate for a performance test.
    QTest::newRow("QTCREATORBUG8974_danglingPointer")
        << _("template <class T>\n"
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
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "    Singleton<INIManager>::instance().bar();\n"
             "}\n")
        << (UseList()
            << Use(1, 17, 1, Highlighting::TypeUse)
            << Use(2, 7, 9, Highlighting::TypeUse)
            << Use(5, 12, 1, Highlighting::TypeUse)
            << Use(5, 15, 8, Highlighting::FunctionUse)
            << Use(8, 6, 3, Highlighting::FunctionUse)
            << Use(10, 6, 3, Highlighting::FunctionUse)
            << Use(12, 5, 9, Highlighting::TypeUse)
            << Use(12, 28, 8, Highlighting::FunctionUse)
            << Use(13, 5, 9, Highlighting::TypeUse)
            << Use(13, 28, 8, Highlighting::FunctionUse)
            << Use(14, 5, 9, Highlighting::TypeUse)
            << Use(14, 28, 8, Highlighting::FunctionUse)
            << Use(15, 5, 9, Highlighting::TypeUse)
            << Use(15, 28, 8, Highlighting::FunctionUse)
            << Use(16, 5, 9, Highlighting::TypeUse)
            << Use(16, 28, 8, Highlighting::FunctionUse)
            << Use(17, 5, 9, Highlighting::TypeUse)
            << Use(17, 28, 8, Highlighting::FunctionUse)
            << Use(18, 5, 9, Highlighting::TypeUse)
            << Use(18, 28, 8, Highlighting::FunctionUse)
            << Use(19, 5, 9, Highlighting::TypeUse)
            << Use(19, 28, 8, Highlighting::FunctionUse)
            << Use(20, 5, 9, Highlighting::TypeUse)
            << Use(20, 28, 8, Highlighting::FunctionUse)
            << Use(21, 5, 9, Highlighting::TypeUse)
            << Use(21, 28, 8, Highlighting::FunctionUse)
            << Use(22, 5, 9, Highlighting::TypeUse)
            << Use(22, 28, 8, Highlighting::FunctionUse)
            << Use(23, 5, 9, Highlighting::TypeUse)
            << Use(23, 28, 8, Highlighting::FunctionUse)
            << Use(24, 5, 9, Highlighting::TypeUse)
            << Use(24, 28, 8, Highlighting::FunctionUse)
            << Use(25, 5, 9, Highlighting::TypeUse)
            << Use(25, 28, 8, Highlighting::FunctionUse)
            << Use(26, 5, 9, Highlighting::TypeUse)
            << Use(26, 28, 8, Highlighting::FunctionUse)
            << Use(27, 5, 9, Highlighting::TypeUse)
            << Use(27, 28, 8, Highlighting::FunctionUse)
            << Use(28, 5, 9, Highlighting::TypeUse)
            << Use(28, 28, 8, Highlighting::FunctionUse)
            << Use(29, 5, 9, Highlighting::TypeUse)
            << Use(29, 28, 8, Highlighting::FunctionUse)
            << Use(30, 5, 9, Highlighting::TypeUse)
            << Use(30, 28, 8, Highlighting::FunctionUse)
            << Use(31, 5, 9, Highlighting::TypeUse)
            << Use(31, 28, 8, Highlighting::FunctionUse)
            << Use(32, 5, 9, Highlighting::TypeUse)
            << Use(32, 28, 8, Highlighting::FunctionUse)
            << Use(33, 5, 9, Highlighting::TypeUse)
            << Use(33, 28, 8, Highlighting::FunctionUse)
            << Use(34, 5, 9, Highlighting::TypeUse)
            << Use(34, 28, 8, Highlighting::FunctionUse)
            << Use(35, 5, 9, Highlighting::TypeUse)
            << Use(35, 28, 8, Highlighting::FunctionUse)
            << Use(36, 5, 9, Highlighting::TypeUse)
            << Use(36, 28, 8, Highlighting::FunctionUse)
            << Use(37, 5, 9, Highlighting::TypeUse)
            << Use(37, 28, 8, Highlighting::FunctionUse)
            << Use(38, 5, 9, Highlighting::TypeUse)
            << Use(38, 28, 8, Highlighting::FunctionUse)
            << Use(39, 5, 9, Highlighting::TypeUse)
            << Use(39, 28, 8, Highlighting::FunctionUse)
            << Use(40, 5, 9, Highlighting::TypeUse)
            << Use(40, 28, 8, Highlighting::FunctionUse)
            << Use(41, 5, 9, Highlighting::TypeUse)
            << Use(41, 28, 8, Highlighting::FunctionUse)
            << Use(42, 5, 9, Highlighting::TypeUse)
            << Use(42, 28, 8, Highlighting::FunctionUse)
            << Use(43, 5, 9, Highlighting::TypeUse)
            << Use(43, 28, 8, Highlighting::FunctionUse)
            << Use(44, 5, 9, Highlighting::TypeUse)
            << Use(44, 28, 8, Highlighting::FunctionUse)
            << Use(45, 5, 9, Highlighting::TypeUse)
            << Use(45, 28, 8, Highlighting::FunctionUse)
            << Use(46, 5, 9, Highlighting::TypeUse)
            << Use(46, 28, 8, Highlighting::FunctionUse)
            << Use(47, 5, 9, Highlighting::TypeUse)
            << Use(47, 28, 8, Highlighting::FunctionUse)
            << Use(48, 5, 9, Highlighting::TypeUse)
            << Use(48, 28, 8, Highlighting::FunctionUse)
            << Use(49, 5, 9, Highlighting::TypeUse)
            << Use(49, 28, 8, Highlighting::FunctionUse)
            << Use(50, 5, 9, Highlighting::TypeUse)
            << Use(50, 28, 8, Highlighting::FunctionUse)
            << Use(51, 5, 9, Highlighting::TypeUse)
            << Use(51, 28, 8, Highlighting::FunctionUse)
            << Use(52, 5, 9, Highlighting::TypeUse)
            << Use(52, 28, 8, Highlighting::FunctionUse)
            << Use(53, 5, 9, Highlighting::TypeUse)
            << Use(53, 28, 8, Highlighting::FunctionUse)
            << Use(54, 5, 9, Highlighting::TypeUse)
            << Use(54, 28, 8, Highlighting::FunctionUse)
            << Use(55, 5, 9, Highlighting::TypeUse)
            << Use(55, 28, 8, Highlighting::FunctionUse)
            << Use(56, 5, 9, Highlighting::TypeUse)
            << Use(56, 28, 8, Highlighting::FunctionUse)
            << Use(57, 5, 9, Highlighting::TypeUse)
            << Use(57, 28, 8, Highlighting::FunctionUse)
            << Use(58, 5, 9, Highlighting::TypeUse)
            << Use(58, 28, 8, Highlighting::FunctionUse)
            << Use(59, 5, 9, Highlighting::TypeUse)
            << Use(59, 28, 8, Highlighting::FunctionUse)
            << Use(60, 5, 9, Highlighting::TypeUse)
            << Use(60, 28, 8, Highlighting::FunctionUse)
            << Use(61, 5, 9, Highlighting::TypeUse)
            << Use(61, 28, 8, Highlighting::FunctionUse)
            << Use(62, 5, 9, Highlighting::TypeUse)
            << Use(62, 28, 8, Highlighting::FunctionUse)
            << Use(63, 5, 9, Highlighting::TypeUse)
            << Use(63, 28, 8, Highlighting::FunctionUse)
            << Use(64, 5, 9, Highlighting::TypeUse)
            << Use(64, 28, 8, Highlighting::FunctionUse)
            << Use(65, 5, 9, Highlighting::TypeUse)
            << Use(65, 28, 8, Highlighting::FunctionUse)
            << Use(66, 5, 9, Highlighting::TypeUse)
            << Use(66, 28, 8, Highlighting::FunctionUse)
            << Use(67, 5, 9, Highlighting::TypeUse)
            << Use(67, 28, 8, Highlighting::FunctionUse)
            << Use(68, 5, 9, Highlighting::TypeUse)
            << Use(68, 28, 8, Highlighting::FunctionUse)
            << Use(69, 5, 9, Highlighting::TypeUse)
            << Use(69, 28, 8, Highlighting::FunctionUse)
            << Use(70, 5, 9, Highlighting::TypeUse)
            << Use(70, 28, 8, Highlighting::FunctionUse)
            << Use(71, 5, 9, Highlighting::TypeUse)
            << Use(71, 28, 8, Highlighting::FunctionUse)
            << Use(72, 5, 9, Highlighting::TypeUse)
            << Use(72, 28, 8, Highlighting::FunctionUse)
            << Use(73, 5, 9, Highlighting::TypeUse)
            << Use(73, 28, 8, Highlighting::FunctionUse)
            << Use(74, 5, 9, Highlighting::TypeUse)
            << Use(74, 28, 8, Highlighting::FunctionUse)
            << Use(75, 5, 9, Highlighting::TypeUse)
            << Use(75, 28, 8, Highlighting::FunctionUse)
            << Use(76, 5, 9, Highlighting::TypeUse)
            << Use(76, 28, 8, Highlighting::FunctionUse)
            << Use(77, 5, 9, Highlighting::TypeUse)
            << Use(77, 28, 8, Highlighting::FunctionUse)
            << Use(78, 5, 9, Highlighting::TypeUse)
            << Use(78, 28, 8, Highlighting::FunctionUse)
            << Use(79, 5, 9, Highlighting::TypeUse)
            << Use(79, 28, 8, Highlighting::FunctionUse)
            << Use(80, 5, 9, Highlighting::TypeUse)
            << Use(80, 28, 8, Highlighting::FunctionUse)
            << Use(81, 5, 9, Highlighting::TypeUse)
            << Use(81, 28, 8, Highlighting::FunctionUse)
            << Use(82, 5, 9, Highlighting::TypeUse)
            << Use(82, 28, 8, Highlighting::FunctionUse)
            << Use(83, 5, 9, Highlighting::TypeUse)
            << Use(83, 28, 8, Highlighting::FunctionUse)
            << Use(84, 5, 9, Highlighting::TypeUse)
            << Use(84, 28, 8, Highlighting::FunctionUse)
            << Use(85, 5, 9, Highlighting::TypeUse)
            << Use(85, 28, 8, Highlighting::FunctionUse)
            << Use(86, 5, 9, Highlighting::TypeUse)
            << Use(86, 28, 8, Highlighting::FunctionUse)
            << Use(87, 5, 9, Highlighting::TypeUse)
            << Use(87, 28, 8, Highlighting::FunctionUse)
            << Use(88, 5, 9, Highlighting::TypeUse)
            << Use(88, 28, 8, Highlighting::FunctionUse)
            << Use(89, 5, 9, Highlighting::TypeUse)
            << Use(89, 28, 8, Highlighting::FunctionUse)
            << Use(90, 5, 9, Highlighting::TypeUse)
            << Use(90, 28, 8, Highlighting::FunctionUse)
            << Use(91, 5, 9, Highlighting::TypeUse)
            << Use(91, 28, 8, Highlighting::FunctionUse)
            << Use(92, 5, 9, Highlighting::TypeUse)
            << Use(92, 28, 8, Highlighting::FunctionUse)
            << Use(93, 5, 9, Highlighting::TypeUse)
            << Use(93, 28, 8, Highlighting::FunctionUse)
            << Use(94, 5, 9, Highlighting::TypeUse)
            << Use(94, 28, 8, Highlighting::FunctionUse)
            << Use(95, 5, 9, Highlighting::TypeUse)
            << Use(95, 28, 8, Highlighting::FunctionUse)
            << Use(96, 5, 9, Highlighting::TypeUse)
            << Use(96, 28, 8, Highlighting::FunctionUse)
            << Use(97, 5, 9, Highlighting::TypeUse)
            << Use(97, 28, 8, Highlighting::FunctionUse)
            << Use(98, 5, 9, Highlighting::TypeUse)
            << Use(98, 28, 8, Highlighting::FunctionUse)
            << Use(99, 5, 9, Highlighting::TypeUse)
            << Use(99, 28, 8, Highlighting::FunctionUse)
            << Use(100, 5, 9, Highlighting::TypeUse)
            << Use(100, 28, 8, Highlighting::FunctionUse)
            << Use(101, 5, 9, Highlighting::TypeUse)
            << Use(101, 28, 8, Highlighting::FunctionUse)
            << Use(102, 5, 9, Highlighting::TypeUse)
            << Use(102, 28, 8, Highlighting::FunctionUse)
            << Use(103, 5, 9, Highlighting::TypeUse)
            << Use(103, 28, 8, Highlighting::FunctionUse)
            << Use(104, 5, 9, Highlighting::TypeUse)
            << Use(104, 28, 8, Highlighting::FunctionUse)
            << Use(105, 5, 9, Highlighting::TypeUse)
            << Use(105, 28, 8, Highlighting::FunctionUse)
            << Use(106, 5, 9, Highlighting::TypeUse)
            << Use(106, 28, 8, Highlighting::FunctionUse)
            << Use(107, 5, 9, Highlighting::TypeUse)
            << Use(107, 28, 8, Highlighting::FunctionUse)
            << Use(108, 5, 9, Highlighting::TypeUse)
            << Use(108, 28, 8, Highlighting::FunctionUse)
            << Use(109, 5, 9, Highlighting::TypeUse)
            << Use(109, 28, 8, Highlighting::FunctionUse)
            << Use(110, 5, 9, Highlighting::TypeUse)
            << Use(110, 28, 8, Highlighting::FunctionUse)
            << Use(111, 5, 9, Highlighting::TypeUse)
            << Use(111, 28, 8, Highlighting::FunctionUse)
            << Use(112, 5, 9, Highlighting::TypeUse)
            << Use(112, 28, 8, Highlighting::FunctionUse)
            << Use(113, 5, 9, Highlighting::TypeUse)
            << Use(113, 28, 8, Highlighting::FunctionUse)
            << Use(114, 5, 9, Highlighting::TypeUse)
            << Use(114, 28, 8, Highlighting::FunctionUse)
            << Use(115, 5, 9, Highlighting::TypeUse)
            << Use(115, 28, 8, Highlighting::FunctionUse)
            << Use(116, 5, 9, Highlighting::TypeUse)
            << Use(116, 28, 8, Highlighting::FunctionUse)
            << Use(117, 5, 9, Highlighting::TypeUse)
            << Use(117, 28, 8, Highlighting::FunctionUse)
            << Use(118, 5, 9, Highlighting::TypeUse)
            << Use(118, 28, 8, Highlighting::FunctionUse)
            << Use(119, 5, 9, Highlighting::TypeUse)
            << Use(119, 28, 8, Highlighting::FunctionUse)
            << Use(120, 5, 9, Highlighting::TypeUse)
            << Use(120, 28, 8, Highlighting::FunctionUse)
            << Use(121, 5, 9, Highlighting::TypeUse)
            << Use(121, 28, 8, Highlighting::FunctionUse)
            << Use(122, 5, 9, Highlighting::TypeUse)
            << Use(122, 28, 8, Highlighting::FunctionUse)
            << Use(123, 5, 9, Highlighting::TypeUse)
            << Use(123, 28, 8, Highlighting::FunctionUse)
            << Use(124, 5, 9, Highlighting::TypeUse)
            << Use(124, 28, 8, Highlighting::FunctionUse)
            << Use(125, 5, 9, Highlighting::TypeUse)
            << Use(125, 28, 8, Highlighting::FunctionUse)
            << Use(126, 5, 9, Highlighting::TypeUse)
            << Use(126, 28, 8, Highlighting::FunctionUse)
            << Use(127, 5, 9, Highlighting::TypeUse)
            << Use(127, 28, 8, Highlighting::FunctionUse)
            << Use(128, 5, 9, Highlighting::TypeUse)
            << Use(128, 28, 8, Highlighting::FunctionUse)
            << Use(129, 5, 9, Highlighting::TypeUse)
            << Use(129, 28, 8, Highlighting::FunctionUse)
            << Use(130, 5, 9, Highlighting::TypeUse)
            << Use(130, 28, 8, Highlighting::FunctionUse)
            << Use(131, 5, 9, Highlighting::TypeUse)
            << Use(131, 28, 8, Highlighting::FunctionUse)
            << Use(132, 5, 9, Highlighting::TypeUse)
            << Use(132, 28, 8, Highlighting::FunctionUse)
            << Use(133, 5, 9, Highlighting::TypeUse)
            << Use(133, 28, 8, Highlighting::FunctionUse)
            << Use(134, 5, 9, Highlighting::TypeUse)
            << Use(134, 28, 8, Highlighting::FunctionUse)
            << Use(135, 5, 9, Highlighting::TypeUse)
            << Use(135, 28, 8, Highlighting::FunctionUse)
            << Use(136, 5, 9, Highlighting::TypeUse)
            << Use(136, 28, 8, Highlighting::FunctionUse)
            << Use(137, 5, 9, Highlighting::TypeUse)
            << Use(137, 28, 8, Highlighting::FunctionUse)
            << Use(138, 5, 9, Highlighting::TypeUse)
            << Use(138, 28, 8, Highlighting::FunctionUse)
            << Use(139, 5, 9, Highlighting::TypeUse)
            << Use(139, 28, 8, Highlighting::FunctionUse)
            << Use(140, 5, 9, Highlighting::TypeUse)
            << Use(140, 28, 8, Highlighting::FunctionUse)
            << Use(141, 5, 9, Highlighting::TypeUse)
            << Use(141, 28, 8, Highlighting::FunctionUse)
            << Use(142, 5, 9, Highlighting::TypeUse)
            << Use(142, 28, 8, Highlighting::FunctionUse)
            << Use(143, 5, 9, Highlighting::TypeUse)
            << Use(143, 28, 8, Highlighting::FunctionUse)
            << Use(144, 5, 9, Highlighting::TypeUse)
            << Use(144, 28, 8, Highlighting::FunctionUse)
            << Use(145, 5, 9, Highlighting::TypeUse)
            << Use(145, 28, 8, Highlighting::FunctionUse)
            << Use(146, 5, 9, Highlighting::TypeUse)
            << Use(146, 28, 8, Highlighting::FunctionUse)
            << Use(147, 5, 9, Highlighting::TypeUse)
            << Use(147, 28, 8, Highlighting::FunctionUse)
            << Use(148, 5, 9, Highlighting::TypeUse)
            << Use(148, 28, 8, Highlighting::FunctionUse)
            << Use(149, 5, 9, Highlighting::TypeUse)
            << Use(149, 28, 8, Highlighting::FunctionUse)
            << Use(150, 5, 9, Highlighting::TypeUse)
            << Use(150, 28, 8, Highlighting::FunctionUse)
            << Use(151, 5, 9, Highlighting::TypeUse)
            << Use(151, 28, 8, Highlighting::FunctionUse)
            << Use(152, 5, 9, Highlighting::TypeUse)
            << Use(152, 28, 8, Highlighting::FunctionUse)
            << Use(153, 5, 9, Highlighting::TypeUse)
            << Use(153, 28, 8, Highlighting::FunctionUse)
            << Use(154, 5, 9, Highlighting::TypeUse)
            << Use(154, 28, 8, Highlighting::FunctionUse)
            << Use(155, 5, 9, Highlighting::TypeUse)
            << Use(155, 28, 8, Highlighting::FunctionUse)
            << Use(156, 5, 9, Highlighting::TypeUse)
            << Use(156, 28, 8, Highlighting::FunctionUse)
            << Use(157, 5, 9, Highlighting::TypeUse)
            << Use(157, 28, 8, Highlighting::FunctionUse)
            << Use(158, 5, 9, Highlighting::TypeUse)
            << Use(158, 28, 8, Highlighting::FunctionUse)
            << Use(159, 5, 9, Highlighting::TypeUse)
            << Use(159, 28, 8, Highlighting::FunctionUse)
            << Use(160, 5, 9, Highlighting::TypeUse)
            << Use(160, 28, 8, Highlighting::FunctionUse)
            << Use(161, 5, 9, Highlighting::TypeUse)
            << Use(161, 28, 8, Highlighting::FunctionUse)
            << Use(162, 5, 9, Highlighting::TypeUse)
            << Use(162, 28, 8, Highlighting::FunctionUse)
            << Use(163, 5, 9, Highlighting::TypeUse)
            << Use(163, 28, 8, Highlighting::FunctionUse)
            << Use(164, 5, 9, Highlighting::TypeUse)
            << Use(164, 28, 8, Highlighting::FunctionUse)
            << Use(165, 5, 9, Highlighting::TypeUse)
            << Use(165, 28, 8, Highlighting::FunctionUse)
            << Use(166, 5, 9, Highlighting::TypeUse)
            << Use(166, 28, 8, Highlighting::FunctionUse)
            << Use(167, 5, 9, Highlighting::TypeUse)
            << Use(167, 28, 8, Highlighting::FunctionUse)
            << Use(168, 5, 9, Highlighting::TypeUse)
            << Use(168, 28, 8, Highlighting::FunctionUse)
            << Use(169, 5, 9, Highlighting::TypeUse)
            << Use(169, 28, 8, Highlighting::FunctionUse)
            << Use(170, 5, 9, Highlighting::TypeUse)
            << Use(170, 28, 8, Highlighting::FunctionUse)
            << Use(171, 5, 9, Highlighting::TypeUse)
            << Use(171, 28, 8, Highlighting::FunctionUse)
            << Use(172, 5, 9, Highlighting::TypeUse)
            << Use(172, 28, 8, Highlighting::FunctionUse)
            << Use(173, 5, 9, Highlighting::TypeUse)
            << Use(173, 28, 8, Highlighting::FunctionUse)
            << Use(174, 5, 9, Highlighting::TypeUse)
            << Use(174, 28, 8, Highlighting::FunctionUse)
            << Use(175, 5, 9, Highlighting::TypeUse)
            << Use(175, 28, 8, Highlighting::FunctionUse)
            << Use(176, 5, 9, Highlighting::TypeUse)
            << Use(176, 28, 8, Highlighting::FunctionUse)
            << Use(177, 5, 9, Highlighting::TypeUse)
            << Use(177, 28, 8, Highlighting::FunctionUse)
            << Use(178, 5, 9, Highlighting::TypeUse)
            << Use(178, 28, 8, Highlighting::FunctionUse)
            << Use(179, 5, 9, Highlighting::TypeUse)
            << Use(179, 28, 8, Highlighting::FunctionUse)
            << Use(180, 5, 9, Highlighting::TypeUse)
            << Use(180, 28, 8, Highlighting::FunctionUse)
            << Use(181, 5, 9, Highlighting::TypeUse)
            << Use(181, 28, 8, Highlighting::FunctionUse)
            << Use(182, 5, 9, Highlighting::TypeUse)
            << Use(182, 28, 8, Highlighting::FunctionUse)
            << Use(183, 5, 9, Highlighting::TypeUse)
            << Use(183, 28, 8, Highlighting::FunctionUse)
            << Use(184, 5, 9, Highlighting::TypeUse)
            << Use(184, 28, 8, Highlighting::FunctionUse)
            << Use(185, 5, 9, Highlighting::TypeUse)
            << Use(185, 28, 8, Highlighting::FunctionUse)
            << Use(186, 5, 9, Highlighting::TypeUse)
            << Use(186, 28, 8, Highlighting::FunctionUse)
            << Use(187, 5, 9, Highlighting::TypeUse)
            << Use(187, 28, 8, Highlighting::FunctionUse)
            << Use(188, 5, 9, Highlighting::TypeUse)
            << Use(188, 28, 8, Highlighting::FunctionUse)
            << Use(189, 5, 9, Highlighting::TypeUse)
            << Use(189, 28, 8, Highlighting::FunctionUse)
            << Use(190, 5, 9, Highlighting::TypeUse)
            << Use(190, 28, 8, Highlighting::FunctionUse)
            << Use(191, 5, 9, Highlighting::TypeUse)
            << Use(191, 28, 8, Highlighting::FunctionUse)
            << Use(192, 5, 9, Highlighting::TypeUse)
            << Use(192, 28, 8, Highlighting::FunctionUse)
            << Use(193, 5, 9, Highlighting::TypeUse)
            << Use(193, 28, 8, Highlighting::FunctionUse)
            << Use(194, 5, 9, Highlighting::TypeUse)
            << Use(194, 28, 8, Highlighting::FunctionUse)
            << Use(195, 5, 9, Highlighting::TypeUse)
            << Use(195, 28, 8, Highlighting::FunctionUse)
            << Use(196, 5, 9, Highlighting::TypeUse)
            << Use(196, 28, 8, Highlighting::FunctionUse)
            << Use(197, 5, 9, Highlighting::TypeUse)
            << Use(197, 28, 8, Highlighting::FunctionUse)
            << Use(198, 5, 9, Highlighting::TypeUse)
            << Use(198, 28, 8, Highlighting::FunctionUse)
            << Use(199, 5, 9, Highlighting::TypeUse)
            << Use(199, 28, 8, Highlighting::FunctionUse)
            << Use(200, 5, 9, Highlighting::TypeUse)
            << Use(200, 28, 8, Highlighting::FunctionUse)
            << Use(201, 5, 9, Highlighting::TypeUse)
            << Use(201, 28, 8, Highlighting::FunctionUse)
            << Use(202, 5, 9, Highlighting::TypeUse)
            << Use(202, 28, 8, Highlighting::FunctionUse)
            << Use(203, 5, 9, Highlighting::TypeUse)
            << Use(203, 28, 8, Highlighting::FunctionUse)
            << Use(204, 5, 9, Highlighting::TypeUse)
            << Use(204, 28, 8, Highlighting::FunctionUse)
            << Use(205, 5, 9, Highlighting::TypeUse)
            << Use(205, 28, 8, Highlighting::FunctionUse)
            << Use(206, 5, 9, Highlighting::TypeUse)
            << Use(206, 28, 8, Highlighting::FunctionUse)
            << Use(207, 5, 9, Highlighting::TypeUse)
            << Use(207, 28, 8, Highlighting::FunctionUse)
            << Use(208, 5, 9, Highlighting::TypeUse)
            << Use(208, 28, 8, Highlighting::FunctionUse)
            << Use(209, 5, 9, Highlighting::TypeUse)
            << Use(209, 28, 8, Highlighting::FunctionUse)
            << Use(210, 5, 9, Highlighting::TypeUse)
            << Use(210, 28, 8, Highlighting::FunctionUse)
            << Use(211, 5, 9, Highlighting::TypeUse)
            << Use(211, 28, 8, Highlighting::FunctionUse)
            << Use(212, 5, 9, Highlighting::TypeUse)
            << Use(212, 28, 8, Highlighting::FunctionUse)
            << Use(213, 5, 9, Highlighting::TypeUse)
            << Use(213, 28, 8, Highlighting::FunctionUse)
            << Use(214, 5, 9, Highlighting::TypeUse)
            << Use(214, 28, 8, Highlighting::FunctionUse)
            << Use(215, 5, 9, Highlighting::TypeUse)
            << Use(215, 28, 8, Highlighting::FunctionUse)
            << Use(216, 5, 9, Highlighting::TypeUse)
            << Use(216, 28, 8, Highlighting::FunctionUse)
            << Use(217, 5, 9, Highlighting::TypeUse)
            << Use(217, 28, 8, Highlighting::FunctionUse)
            << Use(218, 5, 9, Highlighting::TypeUse)
            << Use(218, 28, 8, Highlighting::FunctionUse)
            << Use(219, 5, 9, Highlighting::TypeUse)
            << Use(219, 28, 8, Highlighting::FunctionUse)
            << Use(220, 5, 9, Highlighting::TypeUse)
            << Use(220, 28, 8, Highlighting::FunctionUse)
            << Use(221, 5, 9, Highlighting::TypeUse)
            << Use(221, 28, 8, Highlighting::FunctionUse)
            << Use(222, 5, 9, Highlighting::TypeUse)
            << Use(222, 28, 8, Highlighting::FunctionUse)
            << Use(223, 5, 9, Highlighting::TypeUse)
            << Use(223, 28, 8, Highlighting::FunctionUse)
            << Use(224, 5, 9, Highlighting::TypeUse)
            << Use(224, 28, 8, Highlighting::FunctionUse)
            << Use(225, 5, 9, Highlighting::TypeUse)
            << Use(225, 28, 8, Highlighting::FunctionUse)
            << Use(226, 5, 9, Highlighting::TypeUse)
            << Use(226, 28, 8, Highlighting::FunctionUse)
            << Use(227, 5, 9, Highlighting::TypeUse)
            << Use(227, 28, 8, Highlighting::FunctionUse)
            << Use(228, 5, 9, Highlighting::TypeUse)
            << Use(228, 28, 8, Highlighting::FunctionUse)
            << Use(229, 5, 9, Highlighting::TypeUse)
            << Use(229, 28, 8, Highlighting::FunctionUse)
            << Use(230, 5, 9, Highlighting::TypeUse)
            << Use(230, 28, 8, Highlighting::FunctionUse)
            << Use(231, 5, 9, Highlighting::TypeUse)
            << Use(231, 28, 8, Highlighting::FunctionUse)
            << Use(232, 5, 9, Highlighting::TypeUse)
            << Use(232, 28, 8, Highlighting::FunctionUse)
            << Use(233, 5, 9, Highlighting::TypeUse)
            << Use(233, 28, 8, Highlighting::FunctionUse)
            << Use(234, 5, 9, Highlighting::TypeUse)
            << Use(234, 28, 8, Highlighting::FunctionUse)
            << Use(235, 5, 9, Highlighting::TypeUse)
            << Use(235, 28, 8, Highlighting::FunctionUse)
            << Use(236, 5, 9, Highlighting::TypeUse)
            << Use(236, 28, 8, Highlighting::FunctionUse)
            << Use(237, 5, 9, Highlighting::TypeUse)
            << Use(237, 28, 8, Highlighting::FunctionUse)
            << Use(238, 5, 9, Highlighting::TypeUse)
            << Use(238, 28, 8, Highlighting::FunctionUse)
            << Use(239, 5, 9, Highlighting::TypeUse)
            << Use(239, 28, 8, Highlighting::FunctionUse)
            << Use(240, 5, 9, Highlighting::TypeUse)
            << Use(240, 28, 8, Highlighting::FunctionUse)
            << Use(241, 5, 9, Highlighting::TypeUse)
            << Use(241, 28, 8, Highlighting::FunctionUse)
            << Use(242, 5, 9, Highlighting::TypeUse)
            << Use(242, 28, 8, Highlighting::FunctionUse)
            << Use(243, 5, 9, Highlighting::TypeUse)
            << Use(243, 28, 8, Highlighting::FunctionUse)
            << Use(244, 5, 9, Highlighting::TypeUse)
            << Use(244, 28, 8, Highlighting::FunctionUse)
            << Use(245, 5, 9, Highlighting::TypeUse)
            << Use(245, 28, 8, Highlighting::FunctionUse)
            << Use(246, 5, 9, Highlighting::TypeUse)
            << Use(246, 28, 8, Highlighting::FunctionUse)
            << Use(247, 5, 9, Highlighting::TypeUse)
            << Use(247, 28, 8, Highlighting::FunctionUse));

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
            << Use(8, 14, 8, Highlighting::FunctionUse)
            << Use(8, 35, 1, Highlighting::FieldUse)
            << Use(9, 5, 1, Highlighting::TypeUse)
            << Use(9, 7, 1, Highlighting::FieldUse)
            << Use(13, 6, 3, Highlighting::FunctionUse)
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
            << Use(3, 30, 1, Highlighting::FunctionUse)
            << Use(4, 10, 1, Highlighting::FunctionUse)
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
            << Use(6, 6, 3, Highlighting::FunctionUse));

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
            << Use(12, 10, 3, Highlighting::FunctionUse)
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
            << Use(2, 6, 4, Highlighting::FunctionUse)
            << Use(4, 8, 4, Highlighting::FieldUse)
            << Use(6, 11, 3, Highlighting::TypeUse)
            << Use(7, 16, 4, Highlighting::FieldUse)
            << Use(8, 8, 4, Highlighting::FunctionUse)
            << Use(10, 10, 4, Highlighting::FieldUse)
            << Use(13, 11, 3, Highlighting::TypeUse)
            << Use(15, 27, 4, Highlighting::FieldUse)
            << Use(16, 10, 4, Highlighting::FunctionUse)
            << Use(16, 19, 4, Highlighting::FieldUse)
            << Use(18, 8, 4, Highlighting::FunctionUse)
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
            << Use(1, 5, 3, Highlighting::FunctionUse)
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
            << Use(8, 6, 3, Highlighting::FunctionUse)
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
            << Use(1, 5, 3, Highlighting::FunctionUse)
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
            << Use(5, 6, 3, Highlighting::FunctionUse)
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
            << Use(6, 6, 3, Highlighting::FunctionUse)
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
            << Use(4, 6, 3, Highlighting::FunctionUse)
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
            << Use(5, 6, 3, Highlighting::FunctionUse)
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
            << Use(6, 6, 3, Highlighting::FunctionUse)
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
            << Use(4, 6, 3, Highlighting::FunctionUse)
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
            << Use(1, 6, 3, Highlighting::FunctionUse)
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
            << Use(1, 6, 3, Highlighting::FunctionUse)
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
            << Use(10, 6, 3, Highlighting::FunctionUse)
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
                << Use(6, 6, 3, Highlighting::FunctionUse)
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
                << Use(8, 10, 3, Highlighting::FunctionUse)
                << Use(10, 13, 3, Highlighting::LocalUse)
                );

    QTest::newRow("using_inside_different_namespace_QTCREATORBUG7978")
        << _("class My" TEST_UNICODE_IDENTIFIER "Type { int " TEST_UNICODE_IDENTIFIER "Member; };\n"
             "void f(My" TEST_UNICODE_IDENTIFIER "Type var" TEST_UNICODE_IDENTIFIER ")\n"
             "{ var" TEST_UNICODE_IDENTIFIER "." TEST_UNICODE_IDENTIFIER "Member = 0; }\n")
        << (UseList()
            << Use(1, 7, 10, Highlighting::TypeUse)
            << Use(1, 24, 10, Highlighting::FieldUse)
            << Use(2, 6, 1, Highlighting::FunctionUse)
            << Use(2, 8, 10, Highlighting::TypeUse)
            << Use(2, 19, 7, Highlighting::LocalUse)
            << Use(3, 3, 7, Highlighting::LocalUse)
            << Use(3, 11, 10, Highlighting::FieldUse));

    QTest::newRow("unicodeIdentifier1")
        << _("class My" TEST_UNICODE_IDENTIFIER "Type { int " TEST_UNICODE_IDENTIFIER "Member; };\n"
             "void f(My" TEST_UNICODE_IDENTIFIER "Type var" TEST_UNICODE_IDENTIFIER ")\n"
             "{ var" TEST_UNICODE_IDENTIFIER "." TEST_UNICODE_IDENTIFIER "Member = 0; }\n")
        << (UseList()
            << Use(1, 7, 10, Highlighting::TypeUse)
            << Use(1, 24, 10, Highlighting::FieldUse)
            << Use(2, 6, 1, Highlighting::FunctionUse)
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
            << Use(8, 8, 5, Highlighting::FunctionUse)
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
            << Use(7, 8, 1, Highlighting::FunctionUse)
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
            << Use(2, 5, 1, Highlighting::FunctionUse)
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

void tst_CheckSymbols::test_parentOfBlock()
{
    const QByteArray source = "void C::f()\n"
                              "{\n"
                              "    enum E { e1 };\n"
                              "}\n";

    const Document::Ptr document = TestCase::createDocument(QLatin1String("file1.cpp"), source);
    Snapshot snapshot;
    snapshot.insert(document);
    TestCase::runCheckSymbols(document, snapshot);
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

QTEST_APPLESS_MAIN(tst_CheckSymbols)
#include "tst_checksymbols.moc"
