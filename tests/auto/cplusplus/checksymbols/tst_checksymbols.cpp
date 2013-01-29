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

#include <CppDocument.h>
#include <pp.h>

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

bool operator==(const Use& lhs, const Use& rhs)
{
    return
        lhs.line == rhs.line &&
        lhs.column == rhs.column &&
        lhs.length == rhs.length &&
        lhs.kind == rhs.kind;
}

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
//            qDebug() << QTest::toString(use);
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
        "struct F {\n"
        "    int i;\n"
        "    F() { i = 0; }\n"
        "};\n"
        "int f()\n"
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
        << Use(8, 7, 1, SemanticInfo::FieldUse)
        << Use(8, 5, 1, SemanticInfo::LocalUse);

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_EnumerationUse()
{
    const QByteArray source =
        "enum E { Red, Green, Blue };\n"
        "E e = Red\n";
    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 22, 4, SemanticInfo::EnumerationUse)
        << Use(1, 15, 5, SemanticInfo::EnumerationUse)
        << Use(1, 6, 1, SemanticInfo::TypeUse)
        << Use(1, 10, 3, SemanticInfo::EnumerationUse);

    TestData::check(source, expectedUses);
}

void tst_CheckSymbols::test_checksymbols_VirtualMethodUse()
{
    const QByteArray source =
        "class B {\n"
        "    virtual isThere();\n"
        "};\n"
        "class D: public B {\n"
        "    isThere();\n"
        "};\n";
    const QList<Use> expectedUses = QList<Use>()
        << Use(1, 7, 1, SemanticInfo::TypeUse)
        << Use(2, 13, 7, SemanticInfo::VirtualMethodUse)
        << Use(4, 17, 1, SemanticInfo::TypeUse)
        << Use(4, 7, 1, SemanticInfo::TypeUse)
        << Use(5, 5, 7, SemanticInfo::VirtualMethodUse);

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
        << Use(2, 11, 3, SemanticInfo::MacroUse)
        << Use(2, 5, 1, SemanticInfo::FunctionUse);

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
            << Use(6, 16, 5, SemanticInfo::FieldUse)
            << Use(6, 9, 5, SemanticInfo::TypeUse)
            << Use(7, 14, 3, SemanticInfo::FunctionUse)
            << Use(11, 12, 3, SemanticInfo::FieldUse)
            << Use(11, 5, 5, SemanticInfo::TypeUse)
            << Use(13, 20, 3, SemanticInfo::FunctionUse)
            << Use(13, 6, 5, SemanticInfo::TypeUse)
            << Use(13, 13, 5, SemanticInfo::TypeUse)
            << Use(15, 5, 3, SemanticInfo::FieldUse)
            << Use(16, 12, 3, SemanticInfo::FieldUse)
            << Use(16, 5, 5, SemanticInfo::TypeUse)
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
            << Use(3, 19, 3, SemanticInfo::EnumerationUse)
            << Use(3, 14, 3, SemanticInfo::EnumerationUse)
            << Use(3, 10, 1, SemanticInfo::TypeUse)
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
            << Use(6, 23, 2, SemanticInfo::FieldUse)
            << Use(6, 12, 6, SemanticInfo::TypeUse)
            << Use(6, 29, 6, SemanticInfo::FieldUse)
            << Use(6, 21, 1, SemanticInfo::TypeUse)
            << Use(9, 6, 3, SemanticInfo::FunctionUse)
            << Use(11, 11, 3, SemanticInfo::TypeUse)
            << Use(11, 16, 4, SemanticInfo::LocalUse)
            << Use(11, 5, 5, SemanticInfo::TypeUse)
            << Use(12, 20, 3, SemanticInfo::FieldUse)
            << Use(12, 17, 2, SemanticInfo::FieldUse)
            << Use(12, 10, 6, SemanticInfo::FieldUse)
            << Use(12, 5, 4, SemanticInfo::LocalUse)
            ;

    TestData::check(source, expectedUses);
}

QTEST_APPLESS_MAIN(tst_CheckSymbols)
#include "tst_checksymbols.moc"
