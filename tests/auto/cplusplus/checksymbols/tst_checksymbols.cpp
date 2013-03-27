/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <cplusplus/CppDocument.h>
#include <cplusplus/pp.h>

#include <cpptools/cppchecksymbols.h>
#include <cpptools/cppsemanticinfo.h>
#include <texteditor/semantichighlighter.h>
#include <utils/fileutils.h>

#include <QDebug>
#include <QDir>
#include <QList>
#include <QtTest>

/*!
    Tests CheckSymbols, the "data provider" of the semantic highlighter.
 */

using namespace CPlusPlus;
using namespace CppTools;

typedef CheckSymbols::Use Use;
typedef CheckSymbols::UseKind UseKind;

static QString useKindToString(UseKind useKind)
{
    switch (useKind) {
    case SemanticInfo::Unknown:
        return QLatin1String("SemanticInfo::Unknown");
    case SemanticInfo::TypeUse:
        return QLatin1String("SemanticInfo::TypeUse");
    case SemanticInfo::LocalUse:
        return QLatin1String("SemanticInfo::LocalUse");
    case SemanticInfo::FieldUse:
        return QLatin1String("SemanticInfo::FieldUse");
    case SemanticInfo::EnumerationUse:
        return QLatin1String("SemanticInfo::EnumerationUse");
    case SemanticInfo::VirtualMethodUse:
        return QLatin1String("SemanticInfo::VirtualMethodUse");
    case SemanticInfo::LabelUse:
        return QLatin1String("SemanticInfo::LabelUse");
    case SemanticInfo::MacroUse:
        return QLatin1String("SemanticInfo::MacroUse");
    case SemanticInfo::FunctionUse:
        return QLatin1String("SemanticInfo::FunctionUse");
    case SemanticInfo::PseudoKeywordUse:
        return QLatin1String("SemanticInfo::PseudoKeywordUse");
    default:
        return QLatin1String("Unknown Kind");
    }
}

// The following two functions are "enhancements" for QCOMPARE().
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

namespace {

class TestData
{
public:
    Snapshot snapshot;
    Document::Ptr document;

    TestData(const QByteArray &source,
                    Document::ParseMode parseMode = Document::ParseTranlationUnit)
    {
        // Write source to temprorary file
        const QString filePath = QDir::tempPath() + QLatin1String("/file.h");
        document = Document::create(filePath);
        Utils::FileSaver documentSaver(document->fileName());
        documentSaver.write(source);
        documentSaver.finalize();

        // Preprocess source
        Environment env;
        Preprocessor preprocess(0, &env);
        const QByteArray preprocessedSource = preprocess.run(filePath, QLatin1String(source));
        document->setUtf8Source(preprocessedSource);
        QVERIFY(document->parse(parseMode));
        document->check();
        snapshot.insert(document);
    }

    static void check(QByteArray source, QList<Use> expectedUses,
                      QList<Use> macroUses = QList<Use>())
    {
        TestData env(source);

        // Collect symbols
        LookupContext context(env.document, env.snapshot);
        CheckSymbols::Future future = CheckSymbols::go(env.document, context, macroUses);
        future.waitForFinished();

        const int resultCount = future.resultCount();
        QList<Use> actualUses;
        for (int i = 0; i < resultCount; ++i) {
            const Use use = future.resultAt(i);
            // When adding tests, you may want to uncomment the
            // following line in order to print out all found uses.
            qDebug() << QTest::toString(use);
            actualUses.append(use);
        }

        // Checks
        QVERIFY(resultCount > 0);
        QCOMPARE(resultCount, expectedUses.count());

        for (int i = 0; i < resultCount; ++i) {
            const Use actualUse = actualUses.at(i);
            const Use expectedUse = expectedUses.at(i);
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
    void test_checksymbols_TypeUse();
    void test_checksymbols_LocalUse();
    void test_checksymbols_FieldUse();
    void test_checksymbols_EnumerationUse();
    void test_checksymbols_VirtualMethodUse();
    void test_checksymbols_LabelUse();
    void test_checksymbols_MacroUse();
    void test_checksymbols_FunctionUse();
    void test_checksymbols_PseudoKeywordUse();
    void test_checksymbols_StaticUse();
    void test_checksymbols_VariableHasTheSameNameAsEnumUse();
    void test_checksymbols_NestedClassOfEnclosingTemplateUse();

    void test_checksymbols_QTCREATORBUG8890_danglingPointer();
    void test_checksymbols_QTCREATORBUG8974_danglingPointer();
};

void tst_CheckSymbols::test_checksymbols_TypeUse()
{
    const QByteArray source =
        "namespace N {}\n"
        "using namespace N;\n";
    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 11, 1, SemanticInfo::TypeUse)
        << Use(2, 17, 1, SemanticInfo::TypeUse);

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_LocalUse()
{
    const QByteArray source =
        "int f()\n"
        "{\n"
        "   int i;\n"
        "}\n";
    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 5, 1, SemanticInfo::FunctionUse)
        << Use(3, 8, 1, SemanticInfo::LocalUse);

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_FieldUse()
{
    const QByteArray source =
        "struct F {\n"          // 1
        "    int i;\n"
        "    F() { i = 0; }\n"
        "};\n"
        "int f()\n"             // 5
        "{\n"
        "    F s;\n"
        "    s.i = 2;\n"
        "}\n";
    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 8, 1, SemanticInfo::TypeUse)
        << Use(2, 9, 1, SemanticInfo::FieldUse)
        << Use(3, 5, 1, SemanticInfo::TypeUse)
        << Use(3, 11, 1, SemanticInfo::FieldUse)
        << Use(5, 5, 1, SemanticInfo::FunctionUse)
        << Use(7, 5, 1, SemanticInfo::TypeUse)
        << Use(7, 7, 1, SemanticInfo::LocalUse)
        << Use(8, 5, 1, SemanticInfo::LocalUse)
        << Use(8, 7, 1, SemanticInfo::FieldUse)
           ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_EnumerationUse()
{
    const QByteArray source =
        "enum E { Red, Green, Blue };\n"
        "E e = Red\n";
    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 6, 1, SemanticInfo::TypeUse)
        << Use(1, 10, 3, SemanticInfo::EnumerationUse)
        << Use(1, 15, 5, SemanticInfo::EnumerationUse)
        << Use(1, 22, 4, SemanticInfo::EnumerationUse)
           ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_VirtualMethodUse()
{
    const QByteArray source =
        "class B {\n"                   // 1
        "    virtual bool isThere();\n" // 2
        "};\n"                          // 3
        "class D: public B {\n"         // 4
        "    bool isThere();\n"         // 5
        "};\n";
    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 7, 1, SemanticInfo::TypeUse)              // B
        << Use(2, 18, 7, SemanticInfo::VirtualMethodUse)    // isThere
        << Use(4, 7, 1, SemanticInfo::TypeUse)              // D
        << Use(4, 17, 1, SemanticInfo::TypeUse)             // B
        << Use(5, 10, 7, SemanticInfo::VirtualMethodUse);   // isThere

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_LabelUse()
{
    const QByteArray source =
        "int f()\n"
        "{\n"
        "   goto g;\n"
        "   g: return 1;\n"
        "}\n";
    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 5, 1, SemanticInfo::FunctionUse)
        << Use(3, 9, 1, SemanticInfo::LabelUse)
        << Use(4, 4, 1, SemanticInfo::LabelUse);

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_MacroUse()
{
    const QByteArray source =
        "#define FOO 1+1\n"
        "int f() { FOO }\n";
    const QList<Use> macroUses = QList<Use>()
        << Use(1, 9, 3, SemanticInfo::MacroUse)
        << Use(2, 11, 3, SemanticInfo::MacroUse);
    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 9, 3, SemanticInfo::MacroUse)
        << Use(2, 5, 1, SemanticInfo::FunctionUse)
        << Use(2, 11, 3, SemanticInfo::MacroUse)
           ;

    TestData::check(source, expectedUses, macroUses);
}

void tst_CheckSymbols::test_checksymbols_FunctionUse()
{
    const QByteArray source =
        "int f();\n"
        "int g() { f(); }\n";
    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 5, 1, SemanticInfo::FunctionUse)
        << Use(2, 5, 1, SemanticInfo::FunctionUse)
        << Use(2, 11, 1, SemanticInfo::FunctionUse);

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_PseudoKeywordUse()
{
    const QByteArray source =
        "class D : public B {"
        "   virtual void f() override {}\n"
        "   virtual void f() final {}\n"
        "};\n";
    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 7, 1, SemanticInfo::TypeUse)
        << Use(1, 37, 1, SemanticInfo::VirtualMethodUse)
        << Use(1, 41, 8, SemanticInfo::PseudoKeywordUse)
        << Use(2, 17, 1, SemanticInfo::VirtualMethodUse)
        << Use(2, 21, 5, SemanticInfo::PseudoKeywordUse);

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_StaticUse()
{
    const QByteArray source =
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

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 8, 5, SemanticInfo::TypeUse)
            << Use(3, 16, 3, SemanticInfo::FieldUse)
            << Use(4, 12, 5, SemanticInfo::TypeUse)
            << Use(6, 9, 5, SemanticInfo::TypeUse)
            << Use(6, 16, 5, SemanticInfo::FieldUse)
            << Use(7, 14, 3, SemanticInfo::FunctionUse)
            << Use(11, 5, 5, SemanticInfo::TypeUse)
            << Use(11, 12, 3, SemanticInfo::FieldUse)
            << Use(13, 6, 5, SemanticInfo::TypeUse)
            << Use(13, 13, 5, SemanticInfo::TypeUse)
            << Use(13, 20, 3, SemanticInfo::FunctionUse)
            << Use(15, 5, 3, SemanticInfo::FieldUse)
            << Use(16, 5, 5, SemanticInfo::TypeUse)
            << Use(16, 12, 3, SemanticInfo::FieldUse)
            << Use(17, 5, 5, SemanticInfo::FieldUse)
            << Use(17, 12, 3, SemanticInfo::FieldUse)
               ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_VariableHasTheSameNameAsEnumUse()
{
    const QByteArray source =
            "struct Foo\n"
            "{\n"
            "    enum E { bar, baz };\n"
            "};\n"
            "\n"
            "struct Boo\n"
            "{\n"
            "    int foo;\n"
            "    int bar;\n"
            "    int baz;\n"
            "};\n"
            ;
    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 8, 3, SemanticInfo::TypeUse)
            << Use(3, 10, 1, SemanticInfo::TypeUse)
            << Use(3, 14, 3, SemanticInfo::EnumerationUse)
            << Use(3, 19, 3, SemanticInfo::EnumerationUse)
            << Use(6, 8, 3, SemanticInfo::TypeUse)
            << Use(8, 9, 3, SemanticInfo::FieldUse)
            << Use(9, 9, 3, SemanticInfo::FieldUse)
            << Use(10, 9, 3, SemanticInfo::FieldUse)
               ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_NestedClassOfEnclosingTemplateUse()
{
    const QByteArray source =
            "struct Foo { int bar; };\n"
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
            "}\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 8, 3, SemanticInfo::TypeUse)
            << Use(1, 18, 3, SemanticInfo::FieldUse)
            << Use(3, 19, 1, SemanticInfo::TypeUse)
            << Use(4, 8, 5, SemanticInfo::TypeUse)
            << Use(6, 12, 6, SemanticInfo::TypeUse)
            << Use(6, 21, 1, SemanticInfo::TypeUse)
            << Use(6, 23, 2, SemanticInfo::FieldUse)
            << Use(6, 29, 6, SemanticInfo::FieldUse)
            << Use(9, 6, 3, SemanticInfo::FunctionUse)
            << Use(11, 5, 5, SemanticInfo::TypeUse)
            << Use(11, 11, 3, SemanticInfo::TypeUse)
            << Use(11, 16, 4, SemanticInfo::LocalUse)
            << Use(12, 5, 4, SemanticInfo::LocalUse)
            << Use(12, 10, 6, SemanticInfo::FieldUse)
            << Use(12, 17, 2, SemanticInfo::FieldUse)
            << Use(12, 20, 3, SemanticInfo::FieldUse)
            ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_QTCREATORBUG8890_danglingPointer()
{
    const QByteArray source =
       "template<class T> class QList {\n"
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
        "}\n"
        ;

    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 16, 1, SemanticInfo::TypeUse)
        << Use(1, 25, 5, SemanticInfo::TypeUse)
        << Use(3, 9, 1, SemanticInfo::TypeUse)
        << Use(3, 11, 8, SemanticInfo::FunctionUse)
        << Use(6, 16, 1, SemanticInfo::TypeUse)
        << Use(6, 25, 8, SemanticInfo::TypeUse)
        << Use(8, 9, 1, SemanticInfo::TypeUse)
        << Use(8, 12, 8, SemanticInfo::FunctionUse)
        << Use(11, 7, 3, SemanticInfo::TypeUse)
        << Use(12, 10, 3, SemanticInfo::FunctionUse)
        << Use(15, 6, 1, SemanticInfo::FunctionUse)
        << Use(17, 5, 5, SemanticInfo::TypeUse)
        << Use(17, 11, 8, SemanticInfo::TypeUse)
        << Use(17, 20, 3, SemanticInfo::TypeUse)
        << Use(17, 27, 4, SemanticInfo::LocalUse)
        << Use(18, 5, 4, SemanticInfo::LocalUse)
        << Use(18, 14, 3, SemanticInfo::FunctionUse)
        << Use(19, 5, 4, SemanticInfo::LocalUse)
        << Use(19, 14, 3, SemanticInfo::FunctionUse)
        ;

    TestData::check(source, expectedUses);
}

// TODO: This is a good candidate for a performance test.
void tst_CheckSymbols::test_checksymbols_QTCREATORBUG8974_danglingPointer()
{
    const QByteArray source =
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
        "};\n"
        ;

    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 17, 1, SemanticInfo::TypeUse)
        << Use(2, 7, 9, SemanticInfo::TypeUse)
        << Use(5, 12, 1, SemanticInfo::TypeUse)
        << Use(5, 15, 8, SemanticInfo::FunctionUse)
        << Use(8, 6, 3, SemanticInfo::FunctionUse)
        << Use(10, 6, 3, SemanticInfo::FunctionUse)
        << Use(12, 5, 9, SemanticInfo::TypeUse)
        << Use(12, 28, 8, SemanticInfo::FunctionUse)
        << Use(13, 5, 9, SemanticInfo::TypeUse)
        << Use(13, 28, 8, SemanticInfo::FunctionUse)
        << Use(14, 5, 9, SemanticInfo::TypeUse)
        << Use(14, 28, 8, SemanticInfo::FunctionUse)
        << Use(15, 5, 9, SemanticInfo::TypeUse)
        << Use(15, 28, 8, SemanticInfo::FunctionUse)
        << Use(16, 5, 9, SemanticInfo::TypeUse)
        << Use(16, 28, 8, SemanticInfo::FunctionUse)
        << Use(17, 5, 9, SemanticInfo::TypeUse)
        << Use(17, 28, 8, SemanticInfo::FunctionUse)
        << Use(18, 5, 9, SemanticInfo::TypeUse)
        << Use(18, 28, 8, SemanticInfo::FunctionUse)
        << Use(19, 5, 9, SemanticInfo::TypeUse)
        << Use(19, 28, 8, SemanticInfo::FunctionUse)
        << Use(20, 5, 9, SemanticInfo::TypeUse)
        << Use(20, 28, 8, SemanticInfo::FunctionUse)
        << Use(21, 5, 9, SemanticInfo::TypeUse)
        << Use(21, 28, 8, SemanticInfo::FunctionUse)
        << Use(22, 5, 9, SemanticInfo::TypeUse)
        << Use(22, 28, 8, SemanticInfo::FunctionUse)
        << Use(23, 5, 9, SemanticInfo::TypeUse)
        << Use(23, 28, 8, SemanticInfo::FunctionUse)
        << Use(24, 5, 9, SemanticInfo::TypeUse)
        << Use(24, 28, 8, SemanticInfo::FunctionUse)
        << Use(25, 5, 9, SemanticInfo::TypeUse)
        << Use(25, 28, 8, SemanticInfo::FunctionUse)
        << Use(26, 5, 9, SemanticInfo::TypeUse)
        << Use(26, 28, 8, SemanticInfo::FunctionUse)
        << Use(27, 5, 9, SemanticInfo::TypeUse)
        << Use(27, 28, 8, SemanticInfo::FunctionUse)
        << Use(28, 5, 9, SemanticInfo::TypeUse)
        << Use(28, 28, 8, SemanticInfo::FunctionUse)
        << Use(29, 5, 9, SemanticInfo::TypeUse)
        << Use(29, 28, 8, SemanticInfo::FunctionUse)
        << Use(30, 5, 9, SemanticInfo::TypeUse)
        << Use(30, 28, 8, SemanticInfo::FunctionUse)
        << Use(31, 5, 9, SemanticInfo::TypeUse)
        << Use(31, 28, 8, SemanticInfo::FunctionUse)
        << Use(32, 5, 9, SemanticInfo::TypeUse)
        << Use(32, 28, 8, SemanticInfo::FunctionUse)
        << Use(33, 5, 9, SemanticInfo::TypeUse)
        << Use(33, 28, 8, SemanticInfo::FunctionUse)
        << Use(34, 5, 9, SemanticInfo::TypeUse)
        << Use(34, 28, 8, SemanticInfo::FunctionUse)
        << Use(35, 5, 9, SemanticInfo::TypeUse)
        << Use(35, 28, 8, SemanticInfo::FunctionUse)
        << Use(36, 5, 9, SemanticInfo::TypeUse)
        << Use(36, 28, 8, SemanticInfo::FunctionUse)
        << Use(37, 5, 9, SemanticInfo::TypeUse)
        << Use(37, 28, 8, SemanticInfo::FunctionUse)
        << Use(38, 5, 9, SemanticInfo::TypeUse)
        << Use(38, 28, 8, SemanticInfo::FunctionUse)
        << Use(39, 5, 9, SemanticInfo::TypeUse)
        << Use(39, 28, 8, SemanticInfo::FunctionUse)
        << Use(40, 5, 9, SemanticInfo::TypeUse)
        << Use(40, 28, 8, SemanticInfo::FunctionUse)
        << Use(41, 5, 9, SemanticInfo::TypeUse)
        << Use(41, 28, 8, SemanticInfo::FunctionUse)
        << Use(42, 5, 9, SemanticInfo::TypeUse)
        << Use(42, 28, 8, SemanticInfo::FunctionUse)
        << Use(43, 5, 9, SemanticInfo::TypeUse)
        << Use(43, 28, 8, SemanticInfo::FunctionUse)
        << Use(44, 5, 9, SemanticInfo::TypeUse)
        << Use(44, 28, 8, SemanticInfo::FunctionUse)
        << Use(45, 5, 9, SemanticInfo::TypeUse)
        << Use(45, 28, 8, SemanticInfo::FunctionUse)
        << Use(46, 5, 9, SemanticInfo::TypeUse)
        << Use(46, 28, 8, SemanticInfo::FunctionUse)
        << Use(47, 5, 9, SemanticInfo::TypeUse)
        << Use(47, 28, 8, SemanticInfo::FunctionUse)
        << Use(48, 5, 9, SemanticInfo::TypeUse)
        << Use(48, 28, 8, SemanticInfo::FunctionUse)
        << Use(49, 5, 9, SemanticInfo::TypeUse)
        << Use(49, 28, 8, SemanticInfo::FunctionUse)
        << Use(50, 5, 9, SemanticInfo::TypeUse)
        << Use(50, 28, 8, SemanticInfo::FunctionUse)
        << Use(51, 5, 9, SemanticInfo::TypeUse)
        << Use(51, 28, 8, SemanticInfo::FunctionUse)
        << Use(52, 5, 9, SemanticInfo::TypeUse)
        << Use(52, 28, 8, SemanticInfo::FunctionUse)
        << Use(53, 5, 9, SemanticInfo::TypeUse)
        << Use(53, 28, 8, SemanticInfo::FunctionUse)
        << Use(54, 5, 9, SemanticInfo::TypeUse)
        << Use(54, 28, 8, SemanticInfo::FunctionUse)
        << Use(55, 5, 9, SemanticInfo::TypeUse)
        << Use(55, 28, 8, SemanticInfo::FunctionUse)
        << Use(56, 5, 9, SemanticInfo::TypeUse)
        << Use(56, 28, 8, SemanticInfo::FunctionUse)
        << Use(57, 5, 9, SemanticInfo::TypeUse)
        << Use(57, 28, 8, SemanticInfo::FunctionUse)
        << Use(58, 5, 9, SemanticInfo::TypeUse)
        << Use(58, 28, 8, SemanticInfo::FunctionUse)
        << Use(59, 5, 9, SemanticInfo::TypeUse)
        << Use(59, 28, 8, SemanticInfo::FunctionUse)
        << Use(60, 5, 9, SemanticInfo::TypeUse)
        << Use(60, 28, 8, SemanticInfo::FunctionUse)
        << Use(61, 5, 9, SemanticInfo::TypeUse)
        << Use(61, 28, 8, SemanticInfo::FunctionUse)
        << Use(62, 5, 9, SemanticInfo::TypeUse)
        << Use(62, 28, 8, SemanticInfo::FunctionUse)
        << Use(63, 5, 9, SemanticInfo::TypeUse)
        << Use(63, 28, 8, SemanticInfo::FunctionUse)
        << Use(64, 5, 9, SemanticInfo::TypeUse)
        << Use(64, 28, 8, SemanticInfo::FunctionUse)
        << Use(65, 5, 9, SemanticInfo::TypeUse)
        << Use(65, 28, 8, SemanticInfo::FunctionUse)
        << Use(66, 5, 9, SemanticInfo::TypeUse)
        << Use(66, 28, 8, SemanticInfo::FunctionUse)
        << Use(67, 5, 9, SemanticInfo::TypeUse)
        << Use(67, 28, 8, SemanticInfo::FunctionUse)
        << Use(68, 5, 9, SemanticInfo::TypeUse)
        << Use(68, 28, 8, SemanticInfo::FunctionUse)
        << Use(69, 5, 9, SemanticInfo::TypeUse)
        << Use(69, 28, 8, SemanticInfo::FunctionUse)
        << Use(70, 5, 9, SemanticInfo::TypeUse)
        << Use(70, 28, 8, SemanticInfo::FunctionUse)
        << Use(71, 5, 9, SemanticInfo::TypeUse)
        << Use(71, 28, 8, SemanticInfo::FunctionUse)
        << Use(72, 5, 9, SemanticInfo::TypeUse)
        << Use(72, 28, 8, SemanticInfo::FunctionUse)
        << Use(73, 5, 9, SemanticInfo::TypeUse)
        << Use(73, 28, 8, SemanticInfo::FunctionUse)
        << Use(74, 5, 9, SemanticInfo::TypeUse)
        << Use(74, 28, 8, SemanticInfo::FunctionUse)
        << Use(75, 5, 9, SemanticInfo::TypeUse)
        << Use(75, 28, 8, SemanticInfo::FunctionUse)
        << Use(76, 5, 9, SemanticInfo::TypeUse)
        << Use(76, 28, 8, SemanticInfo::FunctionUse)
        << Use(77, 5, 9, SemanticInfo::TypeUse)
        << Use(77, 28, 8, SemanticInfo::FunctionUse)
        << Use(78, 5, 9, SemanticInfo::TypeUse)
        << Use(78, 28, 8, SemanticInfo::FunctionUse)
        << Use(79, 5, 9, SemanticInfo::TypeUse)
        << Use(79, 28, 8, SemanticInfo::FunctionUse)
        << Use(80, 5, 9, SemanticInfo::TypeUse)
        << Use(80, 28, 8, SemanticInfo::FunctionUse)
        << Use(81, 5, 9, SemanticInfo::TypeUse)
        << Use(81, 28, 8, SemanticInfo::FunctionUse)
        << Use(82, 5, 9, SemanticInfo::TypeUse)
        << Use(82, 28, 8, SemanticInfo::FunctionUse)
        << Use(83, 5, 9, SemanticInfo::TypeUse)
        << Use(83, 28, 8, SemanticInfo::FunctionUse)
        << Use(84, 5, 9, SemanticInfo::TypeUse)
        << Use(84, 28, 8, SemanticInfo::FunctionUse)
        << Use(85, 5, 9, SemanticInfo::TypeUse)
        << Use(85, 28, 8, SemanticInfo::FunctionUse)
        << Use(86, 5, 9, SemanticInfo::TypeUse)
        << Use(86, 28, 8, SemanticInfo::FunctionUse)
        << Use(87, 5, 9, SemanticInfo::TypeUse)
        << Use(87, 28, 8, SemanticInfo::FunctionUse)
        << Use(88, 5, 9, SemanticInfo::TypeUse)
        << Use(88, 28, 8, SemanticInfo::FunctionUse)
        << Use(89, 5, 9, SemanticInfo::TypeUse)
        << Use(89, 28, 8, SemanticInfo::FunctionUse)
        << Use(90, 5, 9, SemanticInfo::TypeUse)
        << Use(90, 28, 8, SemanticInfo::FunctionUse)
        << Use(91, 5, 9, SemanticInfo::TypeUse)
        << Use(91, 28, 8, SemanticInfo::FunctionUse)
        << Use(92, 5, 9, SemanticInfo::TypeUse)
        << Use(92, 28, 8, SemanticInfo::FunctionUse)
        << Use(93, 5, 9, SemanticInfo::TypeUse)
        << Use(93, 28, 8, SemanticInfo::FunctionUse)
        << Use(94, 5, 9, SemanticInfo::TypeUse)
        << Use(94, 28, 8, SemanticInfo::FunctionUse)
        << Use(95, 5, 9, SemanticInfo::TypeUse)
        << Use(95, 28, 8, SemanticInfo::FunctionUse)
        << Use(96, 5, 9, SemanticInfo::TypeUse)
        << Use(96, 28, 8, SemanticInfo::FunctionUse)
        << Use(97, 5, 9, SemanticInfo::TypeUse)
        << Use(97, 28, 8, SemanticInfo::FunctionUse)
        << Use(98, 5, 9, SemanticInfo::TypeUse)
        << Use(98, 28, 8, SemanticInfo::FunctionUse)
        << Use(99, 5, 9, SemanticInfo::TypeUse)
        << Use(99, 28, 8, SemanticInfo::FunctionUse)
        << Use(100, 5, 9, SemanticInfo::TypeUse)
        << Use(100, 28, 8, SemanticInfo::FunctionUse)
        << Use(101, 5, 9, SemanticInfo::TypeUse)
        << Use(101, 28, 8, SemanticInfo::FunctionUse)
        << Use(102, 5, 9, SemanticInfo::TypeUse)
        << Use(102, 28, 8, SemanticInfo::FunctionUse)
        << Use(103, 5, 9, SemanticInfo::TypeUse)
        << Use(103, 28, 8, SemanticInfo::FunctionUse)
        << Use(104, 5, 9, SemanticInfo::TypeUse)
        << Use(104, 28, 8, SemanticInfo::FunctionUse)
        << Use(105, 5, 9, SemanticInfo::TypeUse)
        << Use(105, 28, 8, SemanticInfo::FunctionUse)
        << Use(106, 5, 9, SemanticInfo::TypeUse)
        << Use(106, 28, 8, SemanticInfo::FunctionUse)
        << Use(107, 5, 9, SemanticInfo::TypeUse)
        << Use(107, 28, 8, SemanticInfo::FunctionUse)
        << Use(108, 5, 9, SemanticInfo::TypeUse)
        << Use(108, 28, 8, SemanticInfo::FunctionUse)
        << Use(109, 5, 9, SemanticInfo::TypeUse)
        << Use(109, 28, 8, SemanticInfo::FunctionUse)
        << Use(110, 5, 9, SemanticInfo::TypeUse)
        << Use(110, 28, 8, SemanticInfo::FunctionUse)
        << Use(111, 5, 9, SemanticInfo::TypeUse)
        << Use(111, 28, 8, SemanticInfo::FunctionUse)
        << Use(112, 5, 9, SemanticInfo::TypeUse)
        << Use(112, 28, 8, SemanticInfo::FunctionUse)
        << Use(113, 5, 9, SemanticInfo::TypeUse)
        << Use(113, 28, 8, SemanticInfo::FunctionUse)
        << Use(114, 5, 9, SemanticInfo::TypeUse)
        << Use(114, 28, 8, SemanticInfo::FunctionUse)
        << Use(115, 5, 9, SemanticInfo::TypeUse)
        << Use(115, 28, 8, SemanticInfo::FunctionUse)
        << Use(116, 5, 9, SemanticInfo::TypeUse)
        << Use(116, 28, 8, SemanticInfo::FunctionUse)
        << Use(117, 5, 9, SemanticInfo::TypeUse)
        << Use(117, 28, 8, SemanticInfo::FunctionUse)
        << Use(118, 5, 9, SemanticInfo::TypeUse)
        << Use(118, 28, 8, SemanticInfo::FunctionUse)
        << Use(119, 5, 9, SemanticInfo::TypeUse)
        << Use(119, 28, 8, SemanticInfo::FunctionUse)
        << Use(120, 5, 9, SemanticInfo::TypeUse)
        << Use(120, 28, 8, SemanticInfo::FunctionUse)
        << Use(121, 5, 9, SemanticInfo::TypeUse)
        << Use(121, 28, 8, SemanticInfo::FunctionUse)
        << Use(122, 5, 9, SemanticInfo::TypeUse)
        << Use(122, 28, 8, SemanticInfo::FunctionUse)
        << Use(123, 5, 9, SemanticInfo::TypeUse)
        << Use(123, 28, 8, SemanticInfo::FunctionUse)
        << Use(124, 5, 9, SemanticInfo::TypeUse)
        << Use(124, 28, 8, SemanticInfo::FunctionUse)
        << Use(125, 5, 9, SemanticInfo::TypeUse)
        << Use(125, 28, 8, SemanticInfo::FunctionUse)
        << Use(126, 5, 9, SemanticInfo::TypeUse)
        << Use(126, 28, 8, SemanticInfo::FunctionUse)
        << Use(127, 5, 9, SemanticInfo::TypeUse)
        << Use(127, 28, 8, SemanticInfo::FunctionUse)
        << Use(128, 5, 9, SemanticInfo::TypeUse)
        << Use(128, 28, 8, SemanticInfo::FunctionUse)
        << Use(129, 5, 9, SemanticInfo::TypeUse)
        << Use(129, 28, 8, SemanticInfo::FunctionUse)
        << Use(130, 5, 9, SemanticInfo::TypeUse)
        << Use(130, 28, 8, SemanticInfo::FunctionUse)
        << Use(131, 5, 9, SemanticInfo::TypeUse)
        << Use(131, 28, 8, SemanticInfo::FunctionUse)
        << Use(132, 5, 9, SemanticInfo::TypeUse)
        << Use(132, 28, 8, SemanticInfo::FunctionUse)
        << Use(133, 5, 9, SemanticInfo::TypeUse)
        << Use(133, 28, 8, SemanticInfo::FunctionUse)
        << Use(134, 5, 9, SemanticInfo::TypeUse)
        << Use(134, 28, 8, SemanticInfo::FunctionUse)
        << Use(135, 5, 9, SemanticInfo::TypeUse)
        << Use(135, 28, 8, SemanticInfo::FunctionUse)
        << Use(136, 5, 9, SemanticInfo::TypeUse)
        << Use(136, 28, 8, SemanticInfo::FunctionUse)
        << Use(137, 5, 9, SemanticInfo::TypeUse)
        << Use(137, 28, 8, SemanticInfo::FunctionUse)
        << Use(138, 5, 9, SemanticInfo::TypeUse)
        << Use(138, 28, 8, SemanticInfo::FunctionUse)
        << Use(139, 5, 9, SemanticInfo::TypeUse)
        << Use(139, 28, 8, SemanticInfo::FunctionUse)
        << Use(140, 5, 9, SemanticInfo::TypeUse)
        << Use(140, 28, 8, SemanticInfo::FunctionUse)
        << Use(141, 5, 9, SemanticInfo::TypeUse)
        << Use(141, 28, 8, SemanticInfo::FunctionUse)
        << Use(142, 5, 9, SemanticInfo::TypeUse)
        << Use(142, 28, 8, SemanticInfo::FunctionUse)
        << Use(143, 5, 9, SemanticInfo::TypeUse)
        << Use(143, 28, 8, SemanticInfo::FunctionUse)
        << Use(144, 5, 9, SemanticInfo::TypeUse)
        << Use(144, 28, 8, SemanticInfo::FunctionUse)
        << Use(145, 5, 9, SemanticInfo::TypeUse)
        << Use(145, 28, 8, SemanticInfo::FunctionUse)
        << Use(146, 5, 9, SemanticInfo::TypeUse)
        << Use(146, 28, 8, SemanticInfo::FunctionUse)
        << Use(147, 5, 9, SemanticInfo::TypeUse)
        << Use(147, 28, 8, SemanticInfo::FunctionUse)
        << Use(148, 5, 9, SemanticInfo::TypeUse)
        << Use(148, 28, 8, SemanticInfo::FunctionUse)
        << Use(149, 5, 9, SemanticInfo::TypeUse)
        << Use(149, 28, 8, SemanticInfo::FunctionUse)
        << Use(150, 5, 9, SemanticInfo::TypeUse)
        << Use(150, 28, 8, SemanticInfo::FunctionUse)
        << Use(151, 5, 9, SemanticInfo::TypeUse)
        << Use(151, 28, 8, SemanticInfo::FunctionUse)
        << Use(152, 5, 9, SemanticInfo::TypeUse)
        << Use(152, 28, 8, SemanticInfo::FunctionUse)
        << Use(153, 5, 9, SemanticInfo::TypeUse)
        << Use(153, 28, 8, SemanticInfo::FunctionUse)
        << Use(154, 5, 9, SemanticInfo::TypeUse)
        << Use(154, 28, 8, SemanticInfo::FunctionUse)
        << Use(155, 5, 9, SemanticInfo::TypeUse)
        << Use(155, 28, 8, SemanticInfo::FunctionUse)
        << Use(156, 5, 9, SemanticInfo::TypeUse)
        << Use(156, 28, 8, SemanticInfo::FunctionUse)
        << Use(157, 5, 9, SemanticInfo::TypeUse)
        << Use(157, 28, 8, SemanticInfo::FunctionUse)
        << Use(158, 5, 9, SemanticInfo::TypeUse)
        << Use(158, 28, 8, SemanticInfo::FunctionUse)
        << Use(159, 5, 9, SemanticInfo::TypeUse)
        << Use(159, 28, 8, SemanticInfo::FunctionUse)
        << Use(160, 5, 9, SemanticInfo::TypeUse)
        << Use(160, 28, 8, SemanticInfo::FunctionUse)
        << Use(161, 5, 9, SemanticInfo::TypeUse)
        << Use(161, 28, 8, SemanticInfo::FunctionUse)
        << Use(162, 5, 9, SemanticInfo::TypeUse)
        << Use(162, 28, 8, SemanticInfo::FunctionUse)
        << Use(163, 5, 9, SemanticInfo::TypeUse)
        << Use(163, 28, 8, SemanticInfo::FunctionUse)
        << Use(164, 5, 9, SemanticInfo::TypeUse)
        << Use(164, 28, 8, SemanticInfo::FunctionUse)
        << Use(165, 5, 9, SemanticInfo::TypeUse)
        << Use(165, 28, 8, SemanticInfo::FunctionUse)
        << Use(166, 5, 9, SemanticInfo::TypeUse)
        << Use(166, 28, 8, SemanticInfo::FunctionUse)
        << Use(167, 5, 9, SemanticInfo::TypeUse)
        << Use(167, 28, 8, SemanticInfo::FunctionUse)
        << Use(168, 5, 9, SemanticInfo::TypeUse)
        << Use(168, 28, 8, SemanticInfo::FunctionUse)
        << Use(169, 5, 9, SemanticInfo::TypeUse)
        << Use(169, 28, 8, SemanticInfo::FunctionUse)
        << Use(170, 5, 9, SemanticInfo::TypeUse)
        << Use(170, 28, 8, SemanticInfo::FunctionUse)
        << Use(171, 5, 9, SemanticInfo::TypeUse)
        << Use(171, 28, 8, SemanticInfo::FunctionUse)
        << Use(172, 5, 9, SemanticInfo::TypeUse)
        << Use(172, 28, 8, SemanticInfo::FunctionUse)
        << Use(173, 5, 9, SemanticInfo::TypeUse)
        << Use(173, 28, 8, SemanticInfo::FunctionUse)
        << Use(174, 5, 9, SemanticInfo::TypeUse)
        << Use(174, 28, 8, SemanticInfo::FunctionUse)
        << Use(175, 5, 9, SemanticInfo::TypeUse)
        << Use(175, 28, 8, SemanticInfo::FunctionUse)
        << Use(176, 5, 9, SemanticInfo::TypeUse)
        << Use(176, 28, 8, SemanticInfo::FunctionUse)
        << Use(177, 5, 9, SemanticInfo::TypeUse)
        << Use(177, 28, 8, SemanticInfo::FunctionUse)
        << Use(178, 5, 9, SemanticInfo::TypeUse)
        << Use(178, 28, 8, SemanticInfo::FunctionUse)
        << Use(179, 5, 9, SemanticInfo::TypeUse)
        << Use(179, 28, 8, SemanticInfo::FunctionUse)
        << Use(180, 5, 9, SemanticInfo::TypeUse)
        << Use(180, 28, 8, SemanticInfo::FunctionUse)
        << Use(181, 5, 9, SemanticInfo::TypeUse)
        << Use(181, 28, 8, SemanticInfo::FunctionUse)
        << Use(182, 5, 9, SemanticInfo::TypeUse)
        << Use(182, 28, 8, SemanticInfo::FunctionUse)
        << Use(183, 5, 9, SemanticInfo::TypeUse)
        << Use(183, 28, 8, SemanticInfo::FunctionUse)
        << Use(184, 5, 9, SemanticInfo::TypeUse)
        << Use(184, 28, 8, SemanticInfo::FunctionUse)
        << Use(185, 5, 9, SemanticInfo::TypeUse)
        << Use(185, 28, 8, SemanticInfo::FunctionUse)
        << Use(186, 5, 9, SemanticInfo::TypeUse)
        << Use(186, 28, 8, SemanticInfo::FunctionUse)
        << Use(187, 5, 9, SemanticInfo::TypeUse)
        << Use(187, 28, 8, SemanticInfo::FunctionUse)
        << Use(188, 5, 9, SemanticInfo::TypeUse)
        << Use(188, 28, 8, SemanticInfo::FunctionUse)
        << Use(189, 5, 9, SemanticInfo::TypeUse)
        << Use(189, 28, 8, SemanticInfo::FunctionUse)
        << Use(190, 5, 9, SemanticInfo::TypeUse)
        << Use(190, 28, 8, SemanticInfo::FunctionUse)
        << Use(191, 5, 9, SemanticInfo::TypeUse)
        << Use(191, 28, 8, SemanticInfo::FunctionUse)
        << Use(192, 5, 9, SemanticInfo::TypeUse)
        << Use(192, 28, 8, SemanticInfo::FunctionUse)
        << Use(193, 5, 9, SemanticInfo::TypeUse)
        << Use(193, 28, 8, SemanticInfo::FunctionUse)
        << Use(194, 5, 9, SemanticInfo::TypeUse)
        << Use(194, 28, 8, SemanticInfo::FunctionUse)
        << Use(195, 5, 9, SemanticInfo::TypeUse)
        << Use(195, 28, 8, SemanticInfo::FunctionUse)
        << Use(196, 5, 9, SemanticInfo::TypeUse)
        << Use(196, 28, 8, SemanticInfo::FunctionUse)
        << Use(197, 5, 9, SemanticInfo::TypeUse)
        << Use(197, 28, 8, SemanticInfo::FunctionUse)
        << Use(198, 5, 9, SemanticInfo::TypeUse)
        << Use(198, 28, 8, SemanticInfo::FunctionUse)
        << Use(199, 5, 9, SemanticInfo::TypeUse)
        << Use(199, 28, 8, SemanticInfo::FunctionUse)
        << Use(200, 5, 9, SemanticInfo::TypeUse)
        << Use(200, 28, 8, SemanticInfo::FunctionUse)
        << Use(201, 5, 9, SemanticInfo::TypeUse)
        << Use(201, 28, 8, SemanticInfo::FunctionUse)
        << Use(202, 5, 9, SemanticInfo::TypeUse)
        << Use(202, 28, 8, SemanticInfo::FunctionUse)
        << Use(203, 5, 9, SemanticInfo::TypeUse)
        << Use(203, 28, 8, SemanticInfo::FunctionUse)
        << Use(204, 5, 9, SemanticInfo::TypeUse)
        << Use(204, 28, 8, SemanticInfo::FunctionUse)
        << Use(205, 5, 9, SemanticInfo::TypeUse)
        << Use(205, 28, 8, SemanticInfo::FunctionUse)
        << Use(206, 5, 9, SemanticInfo::TypeUse)
        << Use(206, 28, 8, SemanticInfo::FunctionUse)
        << Use(207, 5, 9, SemanticInfo::TypeUse)
        << Use(207, 28, 8, SemanticInfo::FunctionUse)
        << Use(208, 5, 9, SemanticInfo::TypeUse)
        << Use(208, 28, 8, SemanticInfo::FunctionUse)
        << Use(209, 5, 9, SemanticInfo::TypeUse)
        << Use(209, 28, 8, SemanticInfo::FunctionUse)
        << Use(210, 5, 9, SemanticInfo::TypeUse)
        << Use(210, 28, 8, SemanticInfo::FunctionUse)
        << Use(211, 5, 9, SemanticInfo::TypeUse)
        << Use(211, 28, 8, SemanticInfo::FunctionUse)
        << Use(212, 5, 9, SemanticInfo::TypeUse)
        << Use(212, 28, 8, SemanticInfo::FunctionUse)
        << Use(213, 5, 9, SemanticInfo::TypeUse)
        << Use(213, 28, 8, SemanticInfo::FunctionUse)
        << Use(214, 5, 9, SemanticInfo::TypeUse)
        << Use(214, 28, 8, SemanticInfo::FunctionUse)
        << Use(215, 5, 9, SemanticInfo::TypeUse)
        << Use(215, 28, 8, SemanticInfo::FunctionUse)
        << Use(216, 5, 9, SemanticInfo::TypeUse)
        << Use(216, 28, 8, SemanticInfo::FunctionUse)
        << Use(217, 5, 9, SemanticInfo::TypeUse)
        << Use(217, 28, 8, SemanticInfo::FunctionUse)
        << Use(218, 5, 9, SemanticInfo::TypeUse)
        << Use(218, 28, 8, SemanticInfo::FunctionUse)
        << Use(219, 5, 9, SemanticInfo::TypeUse)
        << Use(219, 28, 8, SemanticInfo::FunctionUse)
        << Use(220, 5, 9, SemanticInfo::TypeUse)
        << Use(220, 28, 8, SemanticInfo::FunctionUse)
        << Use(221, 5, 9, SemanticInfo::TypeUse)
        << Use(221, 28, 8, SemanticInfo::FunctionUse)
        << Use(222, 5, 9, SemanticInfo::TypeUse)
        << Use(222, 28, 8, SemanticInfo::FunctionUse)
        << Use(223, 5, 9, SemanticInfo::TypeUse)
        << Use(223, 28, 8, SemanticInfo::FunctionUse)
        << Use(224, 5, 9, SemanticInfo::TypeUse)
        << Use(224, 28, 8, SemanticInfo::FunctionUse)
        << Use(225, 5, 9, SemanticInfo::TypeUse)
        << Use(225, 28, 8, SemanticInfo::FunctionUse)
        << Use(226, 5, 9, SemanticInfo::TypeUse)
        << Use(226, 28, 8, SemanticInfo::FunctionUse)
        << Use(227, 5, 9, SemanticInfo::TypeUse)
        << Use(227, 28, 8, SemanticInfo::FunctionUse)
        << Use(228, 5, 9, SemanticInfo::TypeUse)
        << Use(228, 28, 8, SemanticInfo::FunctionUse)
        << Use(229, 5, 9, SemanticInfo::TypeUse)
        << Use(229, 28, 8, SemanticInfo::FunctionUse)
        << Use(230, 5, 9, SemanticInfo::TypeUse)
        << Use(230, 28, 8, SemanticInfo::FunctionUse)
        << Use(231, 5, 9, SemanticInfo::TypeUse)
        << Use(231, 28, 8, SemanticInfo::FunctionUse)
        << Use(232, 5, 9, SemanticInfo::TypeUse)
        << Use(232, 28, 8, SemanticInfo::FunctionUse)
        << Use(233, 5, 9, SemanticInfo::TypeUse)
        << Use(233, 28, 8, SemanticInfo::FunctionUse)
        << Use(234, 5, 9, SemanticInfo::TypeUse)
        << Use(234, 28, 8, SemanticInfo::FunctionUse)
        << Use(235, 5, 9, SemanticInfo::TypeUse)
        << Use(235, 28, 8, SemanticInfo::FunctionUse)
        << Use(236, 5, 9, SemanticInfo::TypeUse)
        << Use(236, 28, 8, SemanticInfo::FunctionUse)
        << Use(237, 5, 9, SemanticInfo::TypeUse)
        << Use(237, 28, 8, SemanticInfo::FunctionUse)
        << Use(238, 5, 9, SemanticInfo::TypeUse)
        << Use(238, 28, 8, SemanticInfo::FunctionUse)
        << Use(239, 5, 9, SemanticInfo::TypeUse)
        << Use(239, 28, 8, SemanticInfo::FunctionUse)
        << Use(240, 5, 9, SemanticInfo::TypeUse)
        << Use(240, 28, 8, SemanticInfo::FunctionUse)
        << Use(241, 5, 9, SemanticInfo::TypeUse)
        << Use(241, 28, 8, SemanticInfo::FunctionUse)
        << Use(242, 5, 9, SemanticInfo::TypeUse)
        << Use(242, 28, 8, SemanticInfo::FunctionUse)
        << Use(243, 5, 9, SemanticInfo::TypeUse)
        << Use(243, 28, 8, SemanticInfo::FunctionUse)
        << Use(244, 5, 9, SemanticInfo::TypeUse)
        << Use(244, 28, 8, SemanticInfo::FunctionUse)
        << Use(245, 5, 9, SemanticInfo::TypeUse)
        << Use(245, 28, 8, SemanticInfo::FunctionUse)
        << Use(246, 5, 9, SemanticInfo::TypeUse)
        << Use(246, 28, 8, SemanticInfo::FunctionUse)
        << Use(247, 5, 9, SemanticInfo::TypeUse)
        << Use(247, 28, 8, SemanticInfo::FunctionUse)
        ;

    TestData::check(source, expectedUses);
}

QTEST_APPLESS_MAIN(tst_CheckSymbols)
#include "tst_checksymbols.moc"
