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
#include <cpptools/cpphighlightingsupport.h>
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

typedef CheckSymbols::Result Use;
typedef CheckSymbols::Kind UseKind;

static QString useKindToString(UseKind useKind)
{
    switch (useKind) {
    case CppHighlightingSupport::Unknown:
        return QLatin1String("CppHighlightingSupport::Unknown");
    case CppHighlightingSupport::TypeUse:
        return QLatin1String("CppHighlightingSupport::TypeUse");
    case CppHighlightingSupport::LocalUse:
        return QLatin1String("CppHighlightingSupport::LocalUse");
    case CppHighlightingSupport::FieldUse:
        return QLatin1String("CppHighlightingSupport::FieldUse");
    case CppHighlightingSupport::EnumerationUse:
        return QLatin1String("CppHighlightingSupport::EnumerationUse");
    case CppHighlightingSupport::VirtualMethodUse:
        return QLatin1String("CppHighlightingSupport::VirtualMethodUse");
    case CppHighlightingSupport::LabelUse:
        return QLatin1String("CppHighlightingSupport::LabelUse");
    case CppHighlightingSupport::MacroUse:
        return QLatin1String("CppHighlightingSupport::MacroUse");
    case CppHighlightingSupport::FunctionUse:
        return QLatin1String("CppHighlightingSupport::FunctionUse");
    case CppHighlightingSupport::PseudoKeywordUse:
        return QLatin1String("CppHighlightingSupport::PseudoKeywordUse");
    default:
        QTest::qFail("Unknown UseKind", __FILE__, __LINE__);
        return QLatin1String("Unknown UseKind");
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
    void test_checksymbols_8902_staticFunctionHighlightingAsMember_localVariable();
    void test_checksymbols_8902_staticFunctionHighlightingAsMember_functionArgument();
    void test_checksymbols_8902_staticFunctionHighlightingAsMember_templateParameter();
    void test_checksymbols_8902_staticFunctionHighlightingAsMember_struct();

    void test_checksymbols_QTCREATORBUG8890_danglingPointer();
    void test_checksymbols_QTCREATORBUG8974_danglingPointer();
    void operatorAsteriskOfNestedClassOfTemplateClass_QTCREATORBUG9006();
    void test_checksymbols_templated_functions();
    void test_checksymbols_QTCREATORBUG9098();
    void test_checksymbols_AnonymousClass();
    void test_checksymbols_AnonymousClass_insideNamespace();
    void test_checksymbols_AnonymousClass_QTCREATORBUG8963();
    void test_checksymbols_highlightingTypeWhenUsingNamespaceClass_QTCREATORBUG7903_globalNamespace();
    void test_checksymbols_highlightingTypeWhenUsingNamespaceClass_QTCREATORBUG7903_namespace();
    void test_checksymbols_highlightingTypeWhenUsingNamespaceClass_QTCREATORBUG7903_insideFunction();
    void test_checksymbols_crashWhenUsingNamespaceClass_QTCREATORBUG9323_globalNamespace();
    void test_checksymbols_highlightingUsedTemplateFunctionParameter_QTCREATORBUG6861();
    void test_checksymbols_crashWhenUsingNamespaceClass_QTCREATORBUG9323_namespace();
    void test_checksymbols_crashWhenUsingNamespaceClass_QTCREATORBUG9323_insideFunction();
    void test_alias_decl_QTCREATORBUG9386();
};

void tst_CheckSymbols::test_checksymbols_TypeUse()
{
    const QByteArray source =
        "namespace N {}\n"
        "using namespace N;\n";
    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 11, 1, CppHighlightingSupport::TypeUse)
        << Use(2, 17, 1, CppHighlightingSupport::TypeUse);

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
        << Use(1, 5, 1, CppHighlightingSupport::FunctionUse)
        << Use(3, 8, 1, CppHighlightingSupport::LocalUse);

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
        << Use(1, 8, 1, CppHighlightingSupport::TypeUse)
        << Use(2, 9, 1, CppHighlightingSupport::FieldUse)
        << Use(3, 5, 1, CppHighlightingSupport::TypeUse)
        << Use(3, 11, 1, CppHighlightingSupport::FieldUse)
        << Use(5, 5, 1, CppHighlightingSupport::FunctionUse)
        << Use(7, 5, 1, CppHighlightingSupport::TypeUse)
        << Use(7, 7, 1, CppHighlightingSupport::LocalUse)
        << Use(8, 5, 1, CppHighlightingSupport::LocalUse)
        << Use(8, 7, 1, CppHighlightingSupport::FieldUse)
           ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_EnumerationUse()
{
    const QByteArray source =
        "enum E { Red, Green, Blue };\n"
        "E e = Red\n";
    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 6, 1, CppHighlightingSupport::TypeUse)
        << Use(1, 10, 3, CppHighlightingSupport::EnumerationUse)
        << Use(1, 15, 5, CppHighlightingSupport::EnumerationUse)
        << Use(1, 22, 4, CppHighlightingSupport::EnumerationUse)
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
        << Use(1, 7, 1, CppHighlightingSupport::TypeUse)              // B
        << Use(2, 18, 7, CppHighlightingSupport::VirtualMethodUse)    // isThere
        << Use(4, 7, 1, CppHighlightingSupport::TypeUse)              // D
        << Use(4, 17, 1, CppHighlightingSupport::TypeUse)             // B
        << Use(5, 10, 7, CppHighlightingSupport::VirtualMethodUse);   // isThere

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
        << Use(1, 5, 1, CppHighlightingSupport::FunctionUse)
        << Use(3, 9, 1, CppHighlightingSupport::LabelUse)
        << Use(4, 4, 1, CppHighlightingSupport::LabelUse);

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_MacroUse()
{
    const QByteArray source =
        "#define FOO 1+1\n"
        "int f() { FOO }\n";
    const QList<Use> macroUses = QList<Use>()
        << Use(1, 9, 3, CppHighlightingSupport::MacroUse)
        << Use(2, 11, 3, CppHighlightingSupport::MacroUse);
    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 9, 3, CppHighlightingSupport::MacroUse)
        << Use(2, 5, 1, CppHighlightingSupport::FunctionUse)
        << Use(2, 11, 3, CppHighlightingSupport::MacroUse)
           ;

    TestData::check(source, expectedUses, macroUses);
}

void tst_CheckSymbols::test_checksymbols_FunctionUse()
{
    const QByteArray source =
        "int f();\n"
        "int g() { f(); }\n";
    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 5, 1, CppHighlightingSupport::FunctionUse)
        << Use(2, 5, 1, CppHighlightingSupport::FunctionUse)
        << Use(2, 11, 1, CppHighlightingSupport::FunctionUse);

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
        << Use(1, 7, 1, CppHighlightingSupport::TypeUse)
        << Use(1, 37, 1, CppHighlightingSupport::VirtualMethodUse)
        << Use(1, 41, 8, CppHighlightingSupport::PseudoKeywordUse)
        << Use(2, 17, 1, CppHighlightingSupport::VirtualMethodUse)
        << Use(2, 21, 5, CppHighlightingSupport::PseudoKeywordUse);

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
            << Use(1, 8, 5, CppHighlightingSupport::TypeUse)
            << Use(3, 16, 3, CppHighlightingSupport::FieldUse)
            << Use(4, 12, 5, CppHighlightingSupport::TypeUse)
            << Use(6, 9, 5, CppHighlightingSupport::TypeUse)
            << Use(6, 16, 5, CppHighlightingSupport::FieldUse)
            << Use(7, 14, 3, CppHighlightingSupport::FunctionUse)
            << Use(11, 5, 5, CppHighlightingSupport::TypeUse)
            << Use(11, 12, 3, CppHighlightingSupport::FieldUse)
            << Use(13, 6, 5, CppHighlightingSupport::TypeUse)
            << Use(13, 13, 5, CppHighlightingSupport::TypeUse)
            << Use(13, 20, 3, CppHighlightingSupport::FunctionUse)
            << Use(15, 5, 3, CppHighlightingSupport::FieldUse)
            << Use(16, 5, 5, CppHighlightingSupport::TypeUse)
            << Use(16, 12, 3, CppHighlightingSupport::FieldUse)
            << Use(17, 5, 5, CppHighlightingSupport::FieldUse)
            << Use(17, 12, 3, CppHighlightingSupport::FieldUse)
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
            << Use(1, 8, 3, CppHighlightingSupport::TypeUse)
            << Use(3, 10, 1, CppHighlightingSupport::TypeUse)
            << Use(3, 14, 3, CppHighlightingSupport::EnumerationUse)
            << Use(3, 19, 3, CppHighlightingSupport::EnumerationUse)
            << Use(6, 8, 3, CppHighlightingSupport::TypeUse)
            << Use(8, 9, 3, CppHighlightingSupport::FieldUse)
            << Use(9, 9, 3, CppHighlightingSupport::FieldUse)
            << Use(10, 9, 3, CppHighlightingSupport::FieldUse)
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
            << Use(1, 8, 3, CppHighlightingSupport::TypeUse)
            << Use(1, 18, 3, CppHighlightingSupport::FieldUse)
            << Use(3, 19, 1, CppHighlightingSupport::TypeUse)
            << Use(4, 8, 5, CppHighlightingSupport::TypeUse)
            << Use(6, 12, 6, CppHighlightingSupport::TypeUse)
            << Use(6, 21, 1, CppHighlightingSupport::TypeUse)
            << Use(6, 23, 2, CppHighlightingSupport::FieldUse)
            << Use(6, 29, 6, CppHighlightingSupport::FieldUse)
            << Use(9, 6, 3, CppHighlightingSupport::FunctionUse)
            << Use(11, 5, 5, CppHighlightingSupport::TypeUse)
            << Use(11, 11, 3, CppHighlightingSupport::TypeUse)
            << Use(11, 16, 4, CppHighlightingSupport::LocalUse)
            << Use(12, 5, 4, CppHighlightingSupport::LocalUse)
            << Use(12, 10, 6, CppHighlightingSupport::FieldUse)
            << Use(12, 17, 2, CppHighlightingSupport::FieldUse)
            << Use(12, 20, 3, CppHighlightingSupport::FieldUse)
            ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_8902_staticFunctionHighlightingAsMember_localVariable()
{
    const QByteArray source =
            "struct Foo\n"
            "{\n"
            "    static int foo();\n"
            "};\n"
            "\n"
            "void bar()\n"
            "{\n"
            "    int foo = Foo::foo();\n"
            "}\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 8, 3, CppHighlightingSupport::TypeUse)
            << Use(3, 16, 3, CppHighlightingSupport::FunctionUse)
            << Use(6, 6, 3, CppHighlightingSupport::FunctionUse)
            << Use(8, 9, 3, CppHighlightingSupport::LocalUse)
            << Use(8, 15, 3, CppHighlightingSupport::TypeUse)
            << Use(8, 20, 3, CppHighlightingSupport::FunctionUse)
            ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_8902_staticFunctionHighlightingAsMember_functionArgument()
{
    const QByteArray source =
            "struct Foo\n"
            "{\n"
            "    static int foo();\n"
            "};\n"
            "\n"
            "void bar(int foo)\n"
            "{\n"
            "    Foo::foo();\n"
            "}\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 8, 3, CppHighlightingSupport::TypeUse)
            << Use(3, 16, 3, CppHighlightingSupport::FunctionUse)
            << Use(6, 6, 3, CppHighlightingSupport::FunctionUse)
            << Use(6, 14, 3, CppHighlightingSupport::LocalUse)
            << Use(8, 5, 3, CppHighlightingSupport::TypeUse)
            << Use(8, 10, 3, CppHighlightingSupport::FunctionUse)
            ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_8902_staticFunctionHighlightingAsMember_templateParameter()
{
    const QByteArray source =
            "struct Foo\n"
            "{\n"
            "    static int foo();\n"
            "};\n"
            "\n"
            "template <class foo>\n"
            "void bar()\n"
            "{\n"
            "    Foo::foo();\n"
            "}\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 8, 3, CppHighlightingSupport::TypeUse)
            << Use(3, 16, 3, CppHighlightingSupport::FunctionUse)
            << Use(6, 17, 3, CppHighlightingSupport::TypeUse)
            << Use(7, 6, 3, CppHighlightingSupport::FunctionUse)
            << Use(9, 5, 3, CppHighlightingSupport::TypeUse)
            << Use(9, 10, 3, CppHighlightingSupport::FunctionUse)
            ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_8902_staticFunctionHighlightingAsMember_struct()
{
    const QByteArray source =
            "struct Foo\n"
            "{\n"
            "    static int foo();\n"
            "};\n"
            "\n"
            "struct foo {};\n"
            "void bar()\n"
            "{\n"
            "    Foo::foo();\n"
            "}\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 8, 3, CppHighlightingSupport::TypeUse)
            << Use(3, 16, 3, CppHighlightingSupport::FunctionUse)
            << Use(6, 8, 3, CppHighlightingSupport::TypeUse)
            << Use(7, 6, 3, CppHighlightingSupport::FunctionUse)
            << Use(9, 5, 3, CppHighlightingSupport::TypeUse)
            << Use(9, 10, 3, CppHighlightingSupport::FunctionUse)
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
        << Use(1, 16, 1, CppHighlightingSupport::TypeUse)
        << Use(1, 25, 5, CppHighlightingSupport::TypeUse)
        << Use(3, 9, 1, CppHighlightingSupport::TypeUse)
        << Use(3, 11, 8, CppHighlightingSupport::FunctionUse)
        << Use(6, 16, 1, CppHighlightingSupport::TypeUse)
        << Use(6, 25, 8, CppHighlightingSupport::TypeUse)
        << Use(8, 9, 1, CppHighlightingSupport::TypeUse)
        << Use(8, 12, 8, CppHighlightingSupport::FunctionUse)
        << Use(11, 7, 3, CppHighlightingSupport::TypeUse)
        << Use(12, 10, 3, CppHighlightingSupport::FunctionUse)
        << Use(15, 6, 1, CppHighlightingSupport::FunctionUse)
        << Use(17, 5, 5, CppHighlightingSupport::TypeUse)
        << Use(17, 11, 8, CppHighlightingSupport::TypeUse)
        << Use(17, 20, 3, CppHighlightingSupport::TypeUse)
        << Use(17, 27, 4, CppHighlightingSupport::LocalUse)
        << Use(18, 5, 4, CppHighlightingSupport::LocalUse)
        << Use(18, 14, 3, CppHighlightingSupport::FunctionUse)
        << Use(19, 5, 4, CppHighlightingSupport::LocalUse)
        << Use(19, 14, 3, CppHighlightingSupport::FunctionUse)
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
        << Use(1, 17, 1, CppHighlightingSupport::TypeUse)
        << Use(2, 7, 9, CppHighlightingSupport::TypeUse)
        << Use(5, 12, 1, CppHighlightingSupport::TypeUse)
        << Use(5, 15, 8, CppHighlightingSupport::FunctionUse)
        << Use(8, 6, 3, CppHighlightingSupport::FunctionUse)
        << Use(10, 6, 3, CppHighlightingSupport::FunctionUse)
        << Use(12, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(12, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(13, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(13, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(14, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(14, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(15, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(15, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(16, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(16, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(17, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(17, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(18, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(18, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(19, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(19, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(20, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(20, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(21, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(21, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(22, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(22, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(23, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(23, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(24, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(24, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(25, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(25, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(26, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(26, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(27, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(27, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(28, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(28, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(29, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(29, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(30, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(30, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(31, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(31, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(32, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(32, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(33, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(33, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(34, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(34, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(35, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(35, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(36, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(36, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(37, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(37, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(38, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(38, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(39, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(39, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(40, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(40, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(41, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(41, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(42, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(42, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(43, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(43, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(44, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(44, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(45, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(45, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(46, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(46, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(47, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(47, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(48, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(48, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(49, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(49, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(50, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(50, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(51, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(51, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(52, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(52, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(53, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(53, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(54, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(54, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(55, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(55, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(56, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(56, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(57, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(57, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(58, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(58, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(59, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(59, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(60, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(60, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(61, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(61, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(62, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(62, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(63, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(63, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(64, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(64, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(65, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(65, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(66, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(66, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(67, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(67, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(68, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(68, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(69, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(69, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(70, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(70, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(71, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(71, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(72, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(72, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(73, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(73, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(74, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(74, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(75, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(75, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(76, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(76, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(77, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(77, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(78, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(78, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(79, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(79, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(80, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(80, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(81, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(81, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(82, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(82, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(83, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(83, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(84, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(84, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(85, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(85, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(86, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(86, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(87, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(87, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(88, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(88, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(89, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(89, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(90, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(90, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(91, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(91, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(92, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(92, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(93, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(93, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(94, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(94, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(95, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(95, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(96, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(96, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(97, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(97, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(98, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(98, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(99, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(99, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(100, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(100, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(101, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(101, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(102, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(102, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(103, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(103, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(104, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(104, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(105, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(105, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(106, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(106, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(107, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(107, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(108, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(108, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(109, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(109, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(110, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(110, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(111, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(111, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(112, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(112, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(113, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(113, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(114, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(114, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(115, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(115, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(116, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(116, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(117, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(117, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(118, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(118, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(119, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(119, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(120, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(120, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(121, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(121, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(122, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(122, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(123, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(123, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(124, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(124, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(125, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(125, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(126, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(126, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(127, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(127, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(128, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(128, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(129, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(129, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(130, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(130, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(131, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(131, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(132, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(132, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(133, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(133, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(134, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(134, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(135, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(135, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(136, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(136, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(137, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(137, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(138, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(138, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(139, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(139, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(140, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(140, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(141, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(141, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(142, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(142, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(143, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(143, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(144, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(144, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(145, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(145, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(146, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(146, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(147, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(147, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(148, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(148, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(149, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(149, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(150, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(150, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(151, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(151, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(152, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(152, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(153, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(153, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(154, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(154, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(155, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(155, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(156, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(156, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(157, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(157, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(158, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(158, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(159, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(159, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(160, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(160, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(161, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(161, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(162, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(162, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(163, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(163, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(164, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(164, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(165, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(165, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(166, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(166, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(167, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(167, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(168, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(168, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(169, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(169, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(170, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(170, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(171, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(171, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(172, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(172, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(173, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(173, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(174, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(174, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(175, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(175, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(176, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(176, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(177, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(177, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(178, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(178, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(179, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(179, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(180, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(180, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(181, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(181, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(182, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(182, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(183, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(183, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(184, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(184, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(185, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(185, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(186, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(186, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(187, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(187, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(188, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(188, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(189, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(189, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(190, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(190, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(191, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(191, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(192, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(192, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(193, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(193, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(194, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(194, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(195, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(195, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(196, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(196, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(197, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(197, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(198, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(198, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(199, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(199, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(200, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(200, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(201, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(201, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(202, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(202, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(203, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(203, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(204, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(204, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(205, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(205, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(206, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(206, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(207, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(207, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(208, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(208, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(209, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(209, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(210, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(210, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(211, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(211, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(212, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(212, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(213, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(213, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(214, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(214, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(215, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(215, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(216, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(216, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(217, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(217, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(218, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(218, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(219, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(219, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(220, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(220, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(221, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(221, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(222, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(222, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(223, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(223, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(224, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(224, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(225, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(225, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(226, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(226, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(227, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(227, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(228, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(228, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(229, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(229, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(230, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(230, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(231, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(231, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(232, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(232, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(233, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(233, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(234, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(234, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(235, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(235, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(236, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(236, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(237, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(237, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(238, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(238, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(239, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(239, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(240, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(240, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(241, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(241, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(242, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(242, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(243, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(243, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(244, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(244, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(245, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(245, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(246, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(246, 28, 8, CppHighlightingSupport::FunctionUse)
        << Use(247, 5, 9, CppHighlightingSupport::TypeUse)
        << Use(247, 28, 8, CppHighlightingSupport::FunctionUse)
        ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::operatorAsteriskOfNestedClassOfTemplateClass_QTCREATORBUG9006()
{
    const QByteArray source =
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

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 8, 3, CppHighlightingSupport::TypeUse)
            << Use(1, 18, 3, CppHighlightingSupport::FieldUse)
            << Use(3, 16, 1, CppHighlightingSupport::TypeUse)
            << Use(4, 8, 5, CppHighlightingSupport::TypeUse)
            << Use(6, 10, 6, CppHighlightingSupport::TypeUse)
            << Use(8, 11, 1, CppHighlightingSupport::TypeUse)
            << Use(8, 14, 8, CppHighlightingSupport::FunctionUse)
            << Use(8, 35, 1, CppHighlightingSupport::FieldUse)
            << Use(9, 5, 1, CppHighlightingSupport::TypeUse)
            << Use(9, 7, 1, CppHighlightingSupport::FieldUse)
            << Use(13, 6, 3, CppHighlightingSupport::FunctionUse)
            << Use(15, 3, 5, CppHighlightingSupport::TypeUse)
            << Use(15, 9, 3, CppHighlightingSupport::TypeUse)
            << Use(15, 15, 6, CppHighlightingSupport::TypeUse)
            << Use(15, 22, 6, CppHighlightingSupport::LocalUse)
            << Use(16, 5, 6, CppHighlightingSupport::LocalUse)
            << Use(16, 13, 3, CppHighlightingSupport::FieldUse)
        ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_templated_functions()
{
    const QByteArray source =
            "struct D {};\n"                      // line 1
            "struct A {\n"                        // line 2
            "    template<typename T> int B();\n" // line 3
            "    void C() {\n"                    // line 4
            "        B<D>();\n"                   // line 5
            "        this->B<D>();\n"             // line 6
            "    }\n"                             // line 7
            "};\n"                                // line 8
            ;

    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 8, 1, CppHighlightingSupport::TypeUse)
        << Use(2, 8, 1, CppHighlightingSupport::TypeUse)
        << Use(3, 23, 1, CppHighlightingSupport::TypeUse)
        << Use(3, 30, 1, CppHighlightingSupport::FunctionUse)
        << Use(4, 10, 1, CppHighlightingSupport::FunctionUse)
        << Use(5, 9, 1, CppHighlightingSupport::FunctionUse)
        << Use(5, 11, 1, CppHighlightingSupport::TypeUse)
        << Use(6, 15, 1, CppHighlightingSupport::FunctionUse)
        << Use(6, 17, 1, CppHighlightingSupport::TypeUse)
        ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_AnonymousClass()
{
    const QByteArray source =
            "struct\n"
            "{\n"
            "  int foo;\n"
            "} Foo;\n"
            "\n"
            "void fun()\n"
            "{\n"
            "  foo = 3;\n"
            "}\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(3, 7, 3, CppHighlightingSupport::FieldUse)
            << Use(6, 6, 3, CppHighlightingSupport::FunctionUse)
            ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_QTCREATORBUG9098()
{
    const QByteArray source =
            "template <typename T>\n"
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
            "}\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 20, 1, CppHighlightingSupport::TypeUse)
            << Use(2, 7, 1, CppHighlightingSupport::TypeUse)
            << Use(5, 7, 1, CppHighlightingSupport::TypeUse)
            << Use(5, 10, 1, CppHighlightingSupport::FieldUse)
            << Use(7, 20, 1, CppHighlightingSupport::TypeUse)
            << Use(8, 7, 1, CppHighlightingSupport::TypeUse)
            << Use(11, 5, 1, CppHighlightingSupport::TypeUse)
            << Use(11, 7, 1, CppHighlightingSupport::TypeUse)
            << Use(11, 10, 1, CppHighlightingSupport::FieldUse)
            << Use(12, 10, 3, CppHighlightingSupport::FunctionUse)
            << Use(14, 9, 1, CppHighlightingSupport::FieldUse)
            << Use(14, 11, 1, CppHighlightingSupport::FieldUse)
            ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_AnonymousClass_insideNamespace()
{
    const QByteArray source =
            "struct { int foo1; } Foo1;\n"
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
            "}\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 14, 4, CppHighlightingSupport::FieldUse)
            << Use(2, 6, 4, CppHighlightingSupport::FunctionUse)
            << Use(4, 8, 4, CppHighlightingSupport::FieldUse)
            << Use(6, 11, 3, CppHighlightingSupport::TypeUse)
            << Use(7, 16, 4, CppHighlightingSupport::FieldUse)
            << Use(8, 8, 4, CppHighlightingSupport::FunctionUse)
            << Use(10, 10, 4, CppHighlightingSupport::FieldUse)
            << Use(13, 11, 3, CppHighlightingSupport::TypeUse)
            << Use(15, 27, 4, CppHighlightingSupport::FieldUse)
            << Use(16, 10, 4, CppHighlightingSupport::FunctionUse)
            << Use(18, 8, 4, CppHighlightingSupport::FunctionUse)
            ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_AnonymousClass_QTCREATORBUG8963()
{
    const QByteArray source =
            "typedef enum {\n"
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
            "}\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(2, 5, 5, CppHighlightingSupport::EnumerationUse)
            << Use(3, 3, 8, CppHighlightingSupport::TypeUse)
            << Use(5, 9, 5, CppHighlightingSupport::FieldUse)
            << Use(6, 9, 8, CppHighlightingSupport::FieldUse)
            << Use(7, 3, 6, CppHighlightingSupport::TypeUse)
            << Use(8, 6, 3, CppHighlightingSupport::FunctionUse)
            << Use(10, 5, 6, CppHighlightingSupport::TypeUse)
            << Use(10, 12, 1, CppHighlightingSupport::LocalUse)
            << Use(11, 5, 1, CppHighlightingSupport::LocalUse)
            << Use(11, 7, 5, CppHighlightingSupport::FieldUse)
            << Use(12, 5, 1, CppHighlightingSupport::LocalUse)
            << Use(12, 7, 8, CppHighlightingSupport::FieldUse)
            << Use(13, 5, 5, CppHighlightingSupport::EnumerationUse)
               ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_highlightingTypeWhenUsingNamespaceClass_QTCREATORBUG7903_globalNamespace()
{
    const QByteArray source =
            "namespace NS {\n"
            "class Foo {};\n"
            "}\n"
            "using NS::Foo;\n"
            "void fun()\n"
            "{\n"
            "    Foo foo;\n"
            "}\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 11, 2, CppHighlightingSupport::TypeUse)
            << Use(2, 7, 3, CppHighlightingSupport::TypeUse)
            << Use(4, 7, 2, CppHighlightingSupport::TypeUse)
            << Use(4, 11, 3, CppHighlightingSupport::TypeUse)
            << Use(5, 6, 3, CppHighlightingSupport::FunctionUse)
            << Use(7, 5, 3, CppHighlightingSupport::TypeUse)
            << Use(7, 9, 3, CppHighlightingSupport::LocalUse)
            ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_highlightingTypeWhenUsingNamespaceClass_QTCREATORBUG7903_namespace()
{
    const QByteArray source =
            "namespace NS {\n"
            "class Foo {};\n"
            "}\n"
            "namespace NS1 {\n"
            "using NS::Foo;\n"
            "void fun()\n"
            "{\n"
            "    Foo foo;\n"
            "}\n"
            "}\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 11, 2, CppHighlightingSupport::TypeUse)
            << Use(2, 7, 3, CppHighlightingSupport::TypeUse)
            << Use(4, 11, 3, CppHighlightingSupport::TypeUse)
            << Use(5, 7, 2, CppHighlightingSupport::TypeUse)
            << Use(5, 11, 3, CppHighlightingSupport::TypeUse)
            << Use(6, 6, 3, CppHighlightingSupport::FunctionUse)
            << Use(8, 5, 3, CppHighlightingSupport::TypeUse)
            << Use(8, 9, 3, CppHighlightingSupport::LocalUse)
            ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_highlightingTypeWhenUsingNamespaceClass_QTCREATORBUG7903_insideFunction()
{
    const QByteArray source =
            "namespace NS {\n"
            "class Foo {};\n"
            "}\n"
            "void fun()\n"
            "{\n"
            "    using NS::Foo;\n"
            "    Foo foo;\n"
            "}\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 11, 2, CppHighlightingSupport::TypeUse)
            << Use(2, 7, 3, CppHighlightingSupport::TypeUse)
            << Use(4, 6, 3, CppHighlightingSupport::FunctionUse)
            << Use(6, 11, 2, CppHighlightingSupport::TypeUse)
            << Use(6, 15, 3, CppHighlightingSupport::TypeUse)
            << Use(7, 5, 3, CppHighlightingSupport::TypeUse)
            << Use(7, 9, 3, CppHighlightingSupport::LocalUse)
            ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_crashWhenUsingNamespaceClass_QTCREATORBUG9323_globalNamespace()
{
    const QByteArray source =
            "namespace NS {\n"
            "class Foo {};\n"
            "}\n"
            "using ::;\n"
            "void fun()\n"
            "{\n"
            "    Foo foo;\n"
            "}\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 11, 2, CppHighlightingSupport::TypeUse)
            << Use(2, 7, 3, CppHighlightingSupport::TypeUse)
            << Use(5, 6, 3, CppHighlightingSupport::FunctionUse)
            << Use(7, 9, 3, CppHighlightingSupport::LocalUse)
            ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_crashWhenUsingNamespaceClass_QTCREATORBUG9323_namespace()
{
    const QByteArray source =
            "namespace NS {\n"
            "class Foo {};\n"
            "}\n"
            "namespace NS1 {\n"
            "using ::;\n"
            "void fun()\n"
            "{\n"
            "    Foo foo;\n"
            "}\n"
            "}\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 11, 2, CppHighlightingSupport::TypeUse)
            << Use(2, 7, 3, CppHighlightingSupport::TypeUse)
            << Use(4, 11, 3, CppHighlightingSupport::TypeUse)
            << Use(6, 6, 3, CppHighlightingSupport::FunctionUse)
            << Use(8, 9, 3, CppHighlightingSupport::LocalUse)
            ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_crashWhenUsingNamespaceClass_QTCREATORBUG9323_insideFunction()
{
    const QByteArray source =
            "namespace NS {\n"
            "class Foo {};\n"
            "}\n"
            "void fun()\n"
            "{\n"
            "    using ::;\n"
            "    Foo foo;\n"
            "}\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 11, 2, CppHighlightingSupport::TypeUse)
            << Use(2, 7, 3, CppHighlightingSupport::TypeUse)
            << Use(4, 6, 3, CppHighlightingSupport::FunctionUse)
            << Use(7, 9, 3, CppHighlightingSupport::LocalUse)
            ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_alias_decl_QTCREATORBUG9386()
{
    const QByteArray source =
            "using wobble = int;\n"
            "wobble cobble = 1;\n"
            ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 7, 6, CppHighlightingSupport::TypeUse)
            << Use(2, 1, 6, CppHighlightingSupport::TypeUse)
            ;

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_highlightingUsedTemplateFunctionParameter_QTCREATORBUG6861()
{
    const QByteArray source =
            "template<class TEMP>\n"
            "TEMP \n"
            "foo(TEMP in)\n"
            "{\n"
            "    typename TEMP::type type;\n"
            "}\n"
           ;

    const QList<Use> expectedUses = QList<Use>()
            << Use(1, 16, 4, CppHighlightingSupport::TypeUse)
            << Use(2, 1, 4, CppHighlightingSupport::TypeUse)
            << Use(3, 1, 3, CppHighlightingSupport::FunctionUse)
            << Use(3, 5, 4, CppHighlightingSupport::TypeUse)
            << Use(3, 10, 2, CppHighlightingSupport::LocalUse)
            << Use(5, 14, 4, CppHighlightingSupport::TypeUse)
            << Use(5, 20, 4, CppHighlightingSupport::TypeUse)
            << Use(5, 25, 4, CppHighlightingSupport::LocalUse)
            ;

    TestData::check(source, expectedUses);
}

QTEST_APPLESS_MAIN(tst_CheckSymbols)
#include "tst_checksymbols.moc"
