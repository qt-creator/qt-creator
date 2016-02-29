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

#include <cpptools/cppselectionchanger.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/PreprocessorClient.h>
#include <cplusplus/PreprocessorEnvironment.h>
#include <cplusplus/TranslationUnit.h>
#include <cplusplus/pp-engine.h>

#include <QtTest>
#include <QTextDocument>

using namespace CPlusPlus;
using namespace CppTools;

//TESTED_COMPONENT=src/plugins/cpptools

enum {
    debug = false
};

class ChangeResult {
public:
    ChangeResult() : m_string(""), m_result(false) {}
    ChangeResult(const QString &s) : m_string(s), m_result(true) {}
    ChangeResult(const QString &s, bool r) : m_string(s), m_result(r) {}

    QString string() const { return m_string; }
    bool result() const { return m_result; }

    bool operator==(const ChangeResult& rhs) const
    {
        return string().simplified() == rhs.string().simplified() && result() == rhs.result();
    }

private:
    QString m_string;
    bool m_result;
};

QT_BEGIN_NAMESPACE
namespace QTest {
    template<>
    char *toString(const ChangeResult &changeResult)
    {
        QByteArray ba = QString("\n").toUtf8();
        ba.append(changeResult.string().trimmed().toUtf8());
        ba.append(QString("\nResult: ").toUtf8());
        ba.append(changeResult.result() ? '1' : '0');
        return qstrdup(ba.data());
    }
}
QT_END_NAMESPACE

class tst_CppSelectionChanger : public QObject
{
    Q_OBJECT
public:

private Q_SLOTS:
    void initTestCase();
    void testStructSelection();
    void testFunctionDeclaration();
    void testTemplatedFunctionDeclaration();
    void testNamespace();
    void testClassSelection();
    void testFunctionDefinition();

    void testInstanceDeclaration();
    void testIfStatement();
    void testForSelection();
    void testRangeForSelection();
    void testStringLiteral();
    void testCharLiteral();
    void testFunctionCall();
    void testTemplateId();
    void testLambda();

    void testWholeDocumentSelection();

private:
    static QByteArray preprocess(const QByteArray &source, const QString &filename);
    QTextCursor getCursorStartPositionForFoundString(const QString &stringToSearchFor);

    bool expand(QTextCursor &cursor);
    bool shrink(QTextCursor &cursor);

    bool verifyCursorBeforeChange(const QTextCursor &cursor);
    void doExpand(QTextCursor &cursor, const QString &expectedString);
    void doShrink(QTextCursor &cursor, const QString &expectedString);

    CppSelectionChanger changer;
    QString cppFileString;
    QTextDocument textDocument;
    Document::Ptr cppDocument;
    ChangeResult expected;
    ChangeResult computed;
};

#define STRING_COMPARE_SIMPLIFIED(a, b) QVERIFY(a.simplified() == b.simplified());

QTextCursor tst_CppSelectionChanger::getCursorStartPositionForFoundString(
        const QString &stringToSearchFor)
{
    QTextCursor cursor = textDocument.find(stringToSearchFor);
    // Returns only the start position of the selection, not the whole selection.
    if (!cursor.isNull())
        cursor.setPosition(cursor.anchor());
    return cursor;
}

bool tst_CppSelectionChanger::expand(QTextCursor &cursor)
{
    changer.startChangeSelection();
    bool result =
            changer.changeSelection(
                CppSelectionChanger::ExpandSelection,
                cursor,
                cppDocument);
    changer.stopChangeSelection();

    if (debug)
        qDebug() << "After expansion:" << cursor.selectedText();

    return result;
}

bool tst_CppSelectionChanger::shrink(QTextCursor &cursor)
{
    changer.startChangeSelection();
    bool result =
            changer.changeSelection(
                CppSelectionChanger::ShrinkSelection,
                cursor,
                cppDocument);
    changer.stopChangeSelection();

    if (debug)
        qDebug() << "After shrinking:" << cursor.selectedText();

    return result;
}

bool tst_CppSelectionChanger::verifyCursorBeforeChange(const QTextCursor &cursor)
{
    if (cursor.isNull())
        return false;

    if (debug)
        qDebug() << "Before expansion:" << cursor.selectedText();

    // Set initial cursor, never forget.
    changer.onCursorPositionChanged(cursor);
    return true;
}

#define VERIFY_CHANGE() QCOMPARE(computed, expected)

inline void tst_CppSelectionChanger::doExpand(QTextCursor& cursor, const QString& expectedString)
{
    bool result = expand(cursor);
    computed = ChangeResult(cursor.selectedText(), result);
    expected = ChangeResult(expectedString);
}

inline void tst_CppSelectionChanger::doShrink(QTextCursor& cursor, const QString& expectedString)
{
    bool result = shrink(cursor);
    computed = ChangeResult(cursor.selectedText(), result);
    expected = ChangeResult(expectedString);
}

QByteArray tst_CppSelectionChanger::preprocess(const QByteArray &source, const QString &fileName)
{
    Client *client = 0; // no client.
    Environment env;
    Preprocessor preprocess(client, &env);
    preprocess.setKeepComments(true);
    return preprocess.run(fileName, source);
}

void tst_CppSelectionChanger::initTestCase()
{
    // Read cpp file contents into QTextDocument and CppTools::Document::Ptr.
    QString fileName(SRCDIR "/testCppFile.cpp");
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QTextStream s(&file);
    cppFileString = s.readAll();
    file.close();
    textDocument.setPlainText(cppFileString);

    // Create the CPP document and preprocess the source, just like how the CppEditor does it.
    cppDocument = Document::create(fileName);
    const QByteArray preprocessedSource = preprocess(cppFileString.toUtf8(), fileName);
    cppDocument->setUtf8Source(preprocessedSource);
    cppDocument->parse();
    cppDocument->check();
}

void tst_CppSelectionChanger::testStructSelection()
{
    bool result;
    QTextCursor cursor = getCursorStartPositionForFoundString("int a;");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "int");
    VERIFY_CHANGE();
    doExpand(cursor, "int a;");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                           int a;
                           char b;
                           char x;
                           char y;
                           char z;
                           std::string c;
                           double d;
                           std::map<int, int> e;
                           )");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                           {
                               int a;
                               char b;
                               char x;
                               char y;
                               char z;
                               std::string c;
                               double d;
                               std::map<int, int> e;
                           }
                           )");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                           struct TestClass {
                               int a;
                               char b;
                               char x;
                               char y;
                               char z;
                               std::string c;
                               double d;
                               std::map<int, int> e;
                           }
                           )");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                           struct TestClass {
                               int a;
                               char b;
                               char x;
                               char y;
                               char z;
                               std::string c;
                               double d;
                               std::map<int, int> e;
                           };
                           )");
    VERIFY_CHANGE();
    doShrink(cursor, R"(
                             struct TestClass {
                                 int a;
                                 char b;
                                 char x;
                                 char y;
                                 char z;
                                 std::string c;
                                 double d;
                                 std::map<int, int> e;
                             }
                             )");
    VERIFY_CHANGE();
    doShrink(cursor, R"(
                             {
                                 int a;
                                 char b;
                                 char x;
                                 char y;
                                 char z;
                                 std::string c;
                                 double d;
                                 std::map<int, int> e;
                             }
                             )");
    VERIFY_CHANGE();
    doShrink(cursor, R"(
                             int a;
                             char b;
                             char x;
                             char y;
                             char z;
                             std::string c;
                             double d;
                             std::map<int, int> e;
                             )");
    VERIFY_CHANGE();
    doShrink(cursor, "int a;");
    VERIFY_CHANGE();
    doShrink(cursor, "int");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();

    // Final shrink shouldn't do anything, and return false.
    result = shrink(cursor);
    STRING_COMPARE_SIMPLIFIED(cursor.selectedText(), QString(""));
    QVERIFY(!result);
}

void tst_CppSelectionChanger::testClassSelection()
{
    // Start from class keyword.
    QTextCursor cursor = getCursorStartPositionForFoundString("class CustomClass");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "class");
    VERIFY_CHANGE();
    doExpand(cursor, "class CustomClass");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                           class CustomClass {
                               bool customClassMethod(const int &parameter) const volatile;
                           }
                           )");
    VERIFY_CHANGE();
    doShrink(cursor, "class CustomClass");
    VERIFY_CHANGE();
    doShrink(cursor, "class");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();

    // Start from class name.
    cursor = getCursorStartPositionForFoundString("class CustomClass");
    cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor);
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "CustomClass");
    VERIFY_CHANGE();
    doExpand(cursor, "class CustomClass");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                           class CustomClass {
                               bool customClassMethod(const int &parameter) const volatile;
                           }
                           )");
    VERIFY_CHANGE();
    doShrink(cursor, "class CustomClass");
    VERIFY_CHANGE();
    doShrink(cursor, "CustomClass");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();
}

void tst_CppSelectionChanger::testFunctionDefinition()
{
    QTextCursor cursor = getCursorStartPositionForFoundString("const int &parameter");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "const");
    VERIFY_CHANGE();
    doExpand(cursor, "const int &parameter");
    VERIFY_CHANGE();
    doExpand(cursor, "(const int &parameter)");
    VERIFY_CHANGE();
    doExpand(cursor, "customClassMethod(const int &parameter)");
    VERIFY_CHANGE();
    doExpand(cursor, "customClassMethod(const int &parameter) const volatile");
    VERIFY_CHANGE();
    doExpand(cursor, "bool customClassMethod(const int &parameter) const volatile;");
    VERIFY_CHANGE();
    doShrink(cursor, "customClassMethod(const int &parameter) const volatile");
    VERIFY_CHANGE();
    doShrink(cursor, "customClassMethod(const int &parameter)");
    VERIFY_CHANGE();
    doShrink(cursor, "(const int &parameter)");
    VERIFY_CHANGE();
    doShrink(cursor, "const int &parameter");
    VERIFY_CHANGE();
    doShrink(cursor, "const");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();

    // Test CV qualifiers.
    cursor = getCursorStartPositionForFoundString("const volatile");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "const");
    VERIFY_CHANGE();
    doExpand(cursor, "customClassMethod(const int &parameter) const volatile");
    VERIFY_CHANGE();
    doExpand(cursor, "bool customClassMethod(const int &parameter) const volatile;");
    VERIFY_CHANGE();
    doShrink(cursor, "customClassMethod(const int &parameter) const volatile");
    VERIFY_CHANGE();
    doShrink(cursor, "const");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();

    // Test selection starting from qualified name.
    cursor = getCursorStartPositionForFoundString("CustomClass::customClassMethod");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "CustomClass");
    VERIFY_CHANGE();
    doExpand(cursor, "CustomClass::");
    VERIFY_CHANGE();
    doExpand(cursor, "CustomClass::customClassMethod");
    VERIFY_CHANGE();
    doExpand(cursor, "CustomClass::customClassMethod(const int &parameter)");
    VERIFY_CHANGE();
    doExpand(cursor, "CustomClass::customClassMethod(const int &parameter) const volatile");
    VERIFY_CHANGE();
    doExpand(cursor,
                 "bool CustomClass::customClassMethod(const int &parameter) const volatile");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         bool CustomClass::customClassMethod(const int &parameter) const volatile {
                             int secondParameter = parameter;
                             ++secondParameter;
                             return secondParameter;
                         }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor,
                 "bool CustomClass::customClassMethod(const int &parameter) const volatile");
    VERIFY_CHANGE();
    doShrink(cursor, "CustomClass::customClassMethod(const int &parameter) const volatile");
    VERIFY_CHANGE();
    doShrink(cursor, "CustomClass::customClassMethod(const int &parameter)");
    VERIFY_CHANGE();
    doShrink(cursor, "CustomClass::customClassMethod");
    VERIFY_CHANGE();
    doShrink(cursor, "CustomClass::");
    VERIFY_CHANGE();
    doShrink(cursor, "CustomClass");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();

    // Test selection starting from return value.
    cursor = getCursorStartPositionForFoundString("bool CustomClass::customClassMethod");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "bool");
    VERIFY_CHANGE();
    doExpand(cursor,
                 "bool CustomClass::customClassMethod(const int &parameter) const volatile");
    VERIFY_CHANGE();
    doShrink(cursor, "bool");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();
}

void tst_CppSelectionChanger::testInstanceDeclaration()
{
    QTextCursor cursor = getCursorStartPositionForFoundString("argc, argv");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "argc");
    VERIFY_CHANGE();
    doExpand(cursor, "argc, argv");
    VERIFY_CHANGE();
    doExpand(cursor, "(argc, argv)");
    VERIFY_CHANGE();
    doExpand(cursor, "secondCustomClass(argc, argv)");
    VERIFY_CHANGE();
    doExpand(cursor, "SecondCustomClass secondCustomClass(argc, argv);");
    VERIFY_CHANGE();
    doShrink(cursor, "secondCustomClass(argc, argv)");
    VERIFY_CHANGE();
    doShrink(cursor, "(argc, argv)");
    VERIFY_CHANGE();
    doShrink(cursor, "argc, argv");
    VERIFY_CHANGE();
    doShrink(cursor, "argc");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();

    cursor = getCursorStartPositionForFoundString("SecondCustomClass secondCustomClass");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "SecondCustomClass");
    VERIFY_CHANGE();
    doExpand(cursor, "SecondCustomClass secondCustomClass(argc, argv);");
    VERIFY_CHANGE();
    doShrink(cursor, "SecondCustomClass");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();

    cursor = getCursorStartPositionForFoundString("secondCustomClass(argc, argv)");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "secondCustomClass");
    VERIFY_CHANGE();
    doExpand(cursor, "secondCustomClass(argc, argv)");
    VERIFY_CHANGE();
    doExpand(cursor, "SecondCustomClass secondCustomClass(argc, argv);");
    VERIFY_CHANGE();
    doShrink(cursor, "secondCustomClass(argc, argv)");
    VERIFY_CHANGE();
    doShrink(cursor, "secondCustomClass");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();
}

void tst_CppSelectionChanger::testForSelection()
{
    QTextCursor cursor = getCursorStartPositionForFoundString("int i = 0");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "int");
    VERIFY_CHANGE();
    doExpand(cursor, "int i = 0;");
    VERIFY_CHANGE();
    doExpand(cursor, "int i = 0; i < 3; ++i");
    VERIFY_CHANGE();
    doExpand(cursor, "(int i = 0; i < 3; ++i)");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         for (int i = 0; i < 3; ++i) {
                             std::cout << i;
                         }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, "(int i = 0; i < 3; ++i)");
    VERIFY_CHANGE();
    doShrink(cursor, "int i = 0; i < 3; ++i");
    VERIFY_CHANGE();
    doShrink(cursor, "int i = 0;");
    VERIFY_CHANGE();
    doShrink(cursor, "int");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();
}

void tst_CppSelectionChanger::testRangeForSelection()
{
    QTextCursor cursor = getCursorStartPositionForFoundString("auto val");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "auto");
    VERIFY_CHANGE();
    doExpand(cursor, "auto val :v");
    VERIFY_CHANGE();
    doExpand(cursor, "(auto val :v)");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         for (auto val :v) {
                             std::cout << val;
                         }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, "(auto val :v)");
    VERIFY_CHANGE();
    doShrink(cursor, "auto val :v");
    VERIFY_CHANGE();
    doShrink(cursor, "auto");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();
}

void tst_CppSelectionChanger::testStringLiteral()
{
    QTextCursor cursor = getCursorStartPositionForFoundString("Hello");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "Hello");
    VERIFY_CHANGE();
    doExpand(cursor, "\"Hello\"");
    VERIFY_CHANGE();
    doShrink(cursor, "Hello");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();

    // Test raw string literal.
    cursor = getCursorStartPositionForFoundString("Raw literal");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, R"(

                         Raw literal

                         )");
    VERIFY_CHANGE();
    doExpand(cursor, "\
                         R\"(\
                         Raw literal\
                         )\"\
                         ");
            VERIFY_CHANGE();
    doShrink(cursor, R"(

                         Raw literal

                         )");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();
}

void tst_CppSelectionChanger::testCharLiteral()
{
    QTextCursor cursor = getCursorStartPositionForFoundString("'c'");
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, 1);
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "c");
    VERIFY_CHANGE();
    doExpand(cursor, "'c'");
    VERIFY_CHANGE();
    doShrink(cursor, "c");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();
}

void tst_CppSelectionChanger::testFunctionCall()
{
    QTextCursor cursor = getCursorStartPositionForFoundString("10, 20");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "10");
    VERIFY_CHANGE();
    doExpand(cursor, "10, 20");
    VERIFY_CHANGE();
    doExpand(cursor, "(10, 20)");
    VERIFY_CHANGE();
    doExpand(cursor, "add(10, 20)");
    VERIFY_CHANGE();
    doExpand(cursor, "2, add(10, 20)");
    VERIFY_CHANGE();
    doExpand(cursor, "(2, add(10, 20))");
    VERIFY_CHANGE();
    doExpand(cursor, "add(2, add(10, 20))");
    VERIFY_CHANGE();
    doExpand(cursor, "1, add(2, add(10, 20))");
    VERIFY_CHANGE();
    doExpand(cursor, "(1, add(2, add(10, 20)))");
    VERIFY_CHANGE();
    doExpand(cursor, "add(1, add(2, add(10, 20)))");
    VERIFY_CHANGE();
    doShrink(cursor, "(1, add(2, add(10, 20)))");
    VERIFY_CHANGE();
    doShrink(cursor, "1, add(2, add(10, 20))");
    VERIFY_CHANGE();
    doShrink(cursor, "add(2, add(10, 20))");
    VERIFY_CHANGE();
    doShrink(cursor, "(2, add(10, 20))");
    VERIFY_CHANGE();
    doShrink(cursor, "2, add(10, 20)");
    VERIFY_CHANGE();
    doShrink(cursor, "add(10, 20)");
    VERIFY_CHANGE();
    doShrink(cursor, "(10, 20)");
    VERIFY_CHANGE();
    doShrink(cursor, "10, 20");
    VERIFY_CHANGE();
    doShrink(cursor, "10");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();
}

void tst_CppSelectionChanger::testTemplateId()
{
    // Start from map keyword.
    QTextCursor cursor = getCursorStartPositionForFoundString("std::map<int,TestClass>");
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, 5);
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "map");
    VERIFY_CHANGE();
    doExpand(cursor, "map<int,TestClass>");
    VERIFY_CHANGE();
    doExpand(cursor, "std::map<int,TestClass>");
    VERIFY_CHANGE();
    doExpand(cursor, "std::map<int,TestClass> a;");
    VERIFY_CHANGE();
    doShrink(cursor, "std::map<int,TestClass>");
    VERIFY_CHANGE();
    doShrink(cursor, "map<int,TestClass>");
    VERIFY_CHANGE();
    doShrink(cursor, "map");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();
}

void tst_CppSelectionChanger::testLambda()
{
    QTextCursor cursor = getCursorStartPositionForFoundString("=, &a");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "=, &a");
    VERIFY_CHANGE();
    doExpand(cursor, "[=, &a]");
    VERIFY_CHANGE();
    doExpand(cursor, "[=, &a](int lambdaArgument)");
    VERIFY_CHANGE();
    doExpand(cursor, "[=, &a](int lambdaArgument) -> int");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         [=, &a](int lambdaArgument) -> int {
                                 return lambdaArgument + 1;
                             }
                         )");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         aLambda = [=, &a](int lambdaArgument) -> int {
                                 return lambdaArgument + 1;
                             }
                         )");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         auto aLambda = [=, &a](int lambdaArgument) -> int {
                             return lambdaArgument + 1;
                         };
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, R"(
                         aLambda = [=, &a](int lambdaArgument) -> int {
                                 return lambdaArgument + 1;
                             }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, R"(
                         [=, &a](int lambdaArgument) -> int {
                                 return lambdaArgument + 1;
                             }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, "[=, &a](int lambdaArgument) -> int");
    VERIFY_CHANGE();
    doShrink(cursor, "[=, &a](int lambdaArgument)");
    VERIFY_CHANGE();
    doShrink(cursor, "[=, &a]");
    VERIFY_CHANGE();
    doShrink(cursor, "=, &a");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();

    // Start from inside lambda.
    cursor = getCursorStartPositionForFoundString("return lambdaArgument + 1;");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "return lambdaArgument + 1;");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         {
                                 return lambdaArgument + 1;
                             }
                         )");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         [=, &a](int lambdaArgument) -> int {
                                 return lambdaArgument + 1;
                             }
                         )");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         aLambda = [=, &a](int lambdaArgument) -> int {
                                 return lambdaArgument + 1;
                             }
                         )");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         auto aLambda = [=, &a](int lambdaArgument) -> int {
                             return lambdaArgument + 1;
                         };
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, R"(
                         aLambda = [=, &a](int lambdaArgument) -> int {
                                 return lambdaArgument + 1;
                             }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, R"(
                         [=, &a](int lambdaArgument) -> int {
                                 return lambdaArgument + 1;
                             }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, R"(
                         {
                                 return lambdaArgument + 1;
                             }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, "return lambdaArgument + 1;");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();
}

void tst_CppSelectionChanger::testIfStatement()
{
    // Inside braces.
    QTextCursor cursor = getCursorStartPositionForFoundString("for (int i = 0");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, R"(
                         for (int i = 0; i < 3; ++i) {
                             std::cout << i;
                         }
                         )");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         std::cout << "Hello" << 'c' << 54545 << u8"utf8string" << U"unicodeString";
                         for (int i = 0; i < 3; ++i) {
                             std::cout << i;
                         }

                         for (auto val :v) {
                             std::cout << val;
                         }
                         )");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                    {
                         std::cout << "Hello" << 'c' << 54545 << u8"utf8string" << U"unicodeString";
                         for (int i = 0; i < 3; ++i) {
                             std::cout << i;
                         }

                         for (auto val :v) {
                             std::cout << val;
                         }
                     }
                         )");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                     if (5 == 5) {
                         std::cout << "Hello" << 'c' << 54545 << u8"utf8string" << U"unicodeString";
                         for (int i = 0; i < 3; ++i) {
                             std::cout << i;
                         }

                         for (auto val :v) {
                             std::cout << val;
                         }
                     }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, R"(
                    {
                         std::cout << "Hello" << 'c' << 54545 << u8"utf8string" << U"unicodeString";
                         for (int i = 0; i < 3; ++i) {
                             std::cout << i;
                         }

                         for (auto val :v) {
                             std::cout << val;
                         }
                     }
                          )");
    VERIFY_CHANGE();
    doShrink(cursor, R"(
                         std::cout << "Hello" << 'c' << 54545 << u8"utf8string" << U"unicodeString";
                         for (int i = 0; i < 3; ++i) {
                             std::cout << i;
                         }

                         for (auto val :v) {
                             std::cout << val;
                         }
                          )");
    VERIFY_CHANGE();
    doShrink(cursor, R"(
                         for (int i = 0; i < 3; ++i) {
                             std::cout << i;
                         }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();

    // Inside condition paranthesis.
    cursor = getCursorStartPositionForFoundString("5 == 5");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "5");
    VERIFY_CHANGE();
    doExpand(cursor, "5 == 5");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                     if (5 == 5) {
                         std::cout << "Hello" << 'c' << 54545 << u8"utf8string" << U"unicodeString";
                         for (int i = 0; i < 3; ++i) {
                             std::cout << i;
                         }

                         for (auto val :v) {
                             std::cout << val;
                         }
                     }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, "5 == 5");
    VERIFY_CHANGE();
    doShrink(cursor, "5");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();
}

void tst_CppSelectionChanger::testFunctionDeclaration()
{
    QTextCursor cursor = getCursorStartPositionForFoundString("int a, int b");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "int");
    VERIFY_CHANGE();
    doExpand(cursor, "int a");
    VERIFY_CHANGE();
    doExpand(cursor, "int a, int b");
    VERIFY_CHANGE();
    doExpand(cursor, "(int a, int b)");
    VERIFY_CHANGE();
    doExpand(cursor, "add(int a, int b)");
    VERIFY_CHANGE();
    doExpand(cursor, "int add(int a, int b)");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         int add(int a, int b) {
                             return a + b;
                         }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, "int add(int a, int b)");
    VERIFY_CHANGE();
    doShrink(cursor, "add(int a, int b)");
    VERIFY_CHANGE();
    doShrink(cursor, "(int a, int b)");
    VERIFY_CHANGE();
    doShrink(cursor, "int a, int b");
    VERIFY_CHANGE();
    doShrink(cursor, "int a");
    VERIFY_CHANGE();
    doShrink(cursor, "int");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();
}

void tst_CppSelectionChanger::testTemplatedFunctionDeclaration()
{
    QTextCursor cursor = getCursorStartPositionForFoundString("T a, T b");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "T");
    VERIFY_CHANGE();
    doExpand(cursor, "T a");
    VERIFY_CHANGE();
    doExpand(cursor, "T a, T b");
    VERIFY_CHANGE();
    doExpand(cursor, "(T a, T b)");
    VERIFY_CHANGE();
    doExpand(cursor, "subtract(T a, T b)");
    VERIFY_CHANGE();
    doExpand(cursor, "int subtract(T a, T b)");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         int subtract(T a, T b) {
                             return a - b;
                         }
                         )");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         template <class T>
                         int subtract(T a, T b) {
                             return a - b;
                         }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, R"(
                         int subtract(T a, T b) {
                             return a - b;
                         }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, "int subtract(T a, T b)");
    VERIFY_CHANGE();
    doShrink(cursor, "subtract(T a, T b)");
    VERIFY_CHANGE();
    doShrink(cursor, "(T a, T b)");
    VERIFY_CHANGE();
    doShrink(cursor, "T a, T b");
    VERIFY_CHANGE();
    doShrink(cursor, "T a");
    VERIFY_CHANGE();
    doShrink(cursor, "T");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();

    // Test template selection.
    cursor = getCursorStartPositionForFoundString("template <class T>");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "template");
    VERIFY_CHANGE();
    doExpand(cursor, "template <class T>");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         template <class T>
                         int subtract(T a, T b) {
                             return a - b;
                         }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, "template <class T>");
    VERIFY_CHANGE();
    doShrink(cursor, "template");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();

    // Test template parameter selection.
    cursor = getCursorStartPositionForFoundString("class T");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "class T");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         template <class T>
                         int subtract(T a, T b) {
                             return a - b;
                         }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, "class T");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();
}

void tst_CppSelectionChanger::testNamespace()
{
    QTextCursor cursor = getCursorStartPositionForFoundString("namespace");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "namespace");
    VERIFY_CHANGE();
    doExpand(cursor, "namespace CustomNamespace");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         namespace CustomNamespace {
                             extern int insideNamespace;
                             int foo() {
                                 insideNamespace = 2;
                                 return insideNamespace;
                             }
                         }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, "namespace CustomNamespace");
    VERIFY_CHANGE();
    doShrink(cursor, "namespace");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();

    cursor = getCursorStartPositionForFoundString("CustomNamespace");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    doExpand(cursor, "CustomNamespace");
    VERIFY_CHANGE();
    doExpand(cursor, "namespace CustomNamespace");
    VERIFY_CHANGE();
    doExpand(cursor, R"(
                         namespace CustomNamespace {
                             extern int insideNamespace;
                             int foo() {
                                 insideNamespace = 2;
                                 return insideNamespace;
                             }
                         }
                         )");
    VERIFY_CHANGE();
    doShrink(cursor, "namespace CustomNamespace");
    VERIFY_CHANGE();
    doShrink(cursor, "CustomNamespace");
    VERIFY_CHANGE();
    doShrink(cursor, "");
    VERIFY_CHANGE();
}

void tst_CppSelectionChanger::testWholeDocumentSelection()
{
    bool result;
    QTextCursor cursor = getCursorStartPositionForFoundString("#include <string>");
    QCOMPARE(verifyCursorBeforeChange(cursor), true);

    // Get whole document contents as a string.
    QTextCursor wholeDocumentCursor(cursor);
    wholeDocumentCursor.setPosition(0, QTextCursor::MoveAnchor);
    wholeDocumentCursor.setPosition(
                wholeDocumentCursor.document()->characterCount() - 1, QTextCursor::KeepAnchor);

    // Verify if cursor selected whole document.
    doExpand(cursor, wholeDocumentCursor.selectedText());
    VERIFY_CHANGE();

    // Should fail, because whole document was selected.
    result = expand(cursor);
    STRING_COMPARE_SIMPLIFIED(cursor.selectedText(), wholeDocumentCursor.selectedText());
    QVERIFY(!result);

    // Shrink to no contents.
    doShrink(cursor, "");
    VERIFY_CHANGE();

    // Last shrink should fail.
    result = shrink(cursor);
    STRING_COMPARE_SIMPLIFIED(cursor.selectedText(), QString(""));
    QVERIFY(!result);
}

QTEST_MAIN(tst_CppSelectionChanger)
#include "tst_cppselectionchangertest.moc"
