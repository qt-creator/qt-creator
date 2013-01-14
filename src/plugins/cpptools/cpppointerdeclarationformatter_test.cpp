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

#include "cpptoolsplugin.h"

#include <AST.h>
#include <CppDocument.h>
#include <Symbols.h>
#include <TranslationUnit.h>
#include <pp.h>

#include <cpptools/cpppointerdeclarationformatter.h>
#include <cpptools/cpprefactoringchanges.h>
#include <cpptools/cpptoolsplugin.h>
#include <texteditor/plaintexteditor.h>
#include <utils/changeset.h>
#include <utils/fileutils.h>

#include <QDebug>
#include <QDir>
#include <QTextCursor>
#include <QTextDocument>
#include <QtTest>

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::Internal;

using Utils::ChangeSet;
typedef Utils::ChangeSet::Range Range;

Q_DECLARE_METATYPE(Overview)

static QString stripCursor(const QString &source)
{
    QString copy(source);
    const int pos = copy.indexOf(QLatin1Char('@'));
    if (pos != -1)
        copy.remove(pos, 1);
    else
        qDebug() << Q_FUNC_INFO << "Warning: No cursor marker to strip.";
    return copy;
}

struct TestEnvironment
{
    QByteArray source;
    Snapshot snapshot;
    CppRefactoringFilePtr cppRefactoringFile;
    TextEditor::BaseTextEditorWidget *editor;
    Document::Ptr document;
    QTextDocument *textDocument;
    TranslationUnit *translationUnit;
    Environment env;

    TestEnvironment(const QByteArray &source, Document::ParseMode parseMode)
    {
        // Find cursor position and remove cursor marker '@'
        int cursorPosition = 0;
        QString sourceWithoutCursorMarker = QLatin1String(source);
        const int pos = sourceWithoutCursorMarker.indexOf(QLatin1Char('@'));
        if (pos != -1) {
            sourceWithoutCursorMarker.remove(pos, 1);
            cursorPosition = pos;
        }

        // Write source to temprorary file
        const QString filePath = QDir::tempPath() + QLatin1String("/file.h");
        document = Document::create(filePath);
        Utils::FileSaver documentSaver(document->fileName());
        documentSaver.write(sourceWithoutCursorMarker.toLatin1());
        documentSaver.finalize();

        // Preprocess source
        Preprocessor preprocess(0, &env);
        const QByteArray preprocessedSource = preprocess.run(filePath, sourceWithoutCursorMarker);

        document->setUtf8Source(preprocessedSource);
        document->parse(parseMode);
        document->check();
        translationUnit = document->translationUnit();
        snapshot.insert(document);
        editor = new TextEditor::PlainTextEditorWidget(0);
        QString error;
        editor->open(&error, document->fileName(), document->fileName());

        // Set cursor position
        QTextCursor cursor = editor->textCursor();
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, cursorPosition);
        editor->setTextCursor(cursor);

        textDocument = editor->document();
        cppRefactoringFile = CppRefactoringChanges::file(editor, document);
    }

    void applyFormatting(AST *ast, PointerDeclarationFormatter::CursorHandling cursorHandling)
    {
        Overview overview;
        overview.showReturnTypes = true;
        overview.showArgumentNames = true;
        overview.starBindFlags = Overview::StarBindFlags(0);

        // Run the formatter
        PointerDeclarationFormatter formatter(cppRefactoringFile, overview, cursorHandling);
        ChangeSet change = formatter.format(ast); // ChangeSet may be empty.

        // Apply change
        QTextCursor cursor(textDocument);
        change.apply(&cursor);
    }
};

void CppToolsPlugin::test_format_pointerdeclaration_in_simpledeclarations()
{
    QFETCH(QString, source);
    QFETCH(QString, reformattedSource);

    TestEnvironment env(source.toLatin1(), Document::ParseDeclaration);
    AST *ast = env.translationUnit->ast();
    QVERIFY(ast);

    env.applyFormatting(ast, PointerDeclarationFormatter::RespectCursor);

    QCOMPARE(env.textDocument->toPlainText(), reformattedSource);
}

void CppToolsPlugin::test_format_pointerdeclaration_in_simpledeclarations_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("reformattedSource");

    QString source;

    //
    // Naming scheme for the test cases: <description>_(in|out)-(start|middle|end)
    //   in/out:
    //      Denotes whether the cursor is in or out of the quickfix activation range
    //   start/middle/end:
    //      Denotes whether is cursor is near to the range start, middle or end
    //

    QTest::newRow("basic_in-start")
        << "@char *s;"
        << "char * s;";

    source = QLatin1String("char *s;@");
    QTest::newRow("basic_out-end")
        << source << stripCursor(source);

    QTest::newRow("basic_in-end")
        << "char *s@;"
        << "char * s;";

    QTest::newRow("basic-with-ws_in-start")
        << "\n  @char *s;  " // Add some whitespace to check position detection.
        << "\n  char * s;  ";

    source = QLatin1String("\n @ char *s;  ");
    QTest::newRow("basic-with-ws_out-start")
        << source << stripCursor(source);

    QTest::newRow("basic-with-const_in-start")
        << "@const char *s;"
        << "const char * s;";

    QTest::newRow("basic-with-const-and-init-value_in-end")
        << "const char *s@ = 0;"
        << "const char * s = 0;";

    source = QLatin1String("const char *s @= 0;");
    QTest::newRow("basic-with-const-and-init-value_out-end")
        << source << stripCursor(source);

    QTest::newRow("first-declarator_in-start")
        << "@const char *s, *t;"
        << "const char * s, *t;";

    QTest::newRow("first-declarator_in-end")
        << "const char *s@, *t;"
        << "const char * s, *t;";

    source = QLatin1String("const char *s,@ *t;");
    QTest::newRow("first-declarator_out-end")
        << source << stripCursor(source);

    QTest::newRow("function-declaration-param_in-start")
        << "int f(@char *s);"
        << "int f(char * s);";

    QTest::newRow("function-declaration-param_in-end")
        << "int f(char *s@);"
        << "int f(char * s);";

    QTest::newRow("function-declaration-param_in-end")
        << "int f(char *s@ = 0);"
        << "int f(char * s = 0);";

    QTest::newRow("function-declaration-param-multiple-params_in-end")
        << "int f(char *s@, int);"
        << "int f(char * s, int);";

    source = QLatin1String("int f(char *s)@;");
    QTest::newRow("function-declaration-param_out-end")
        << source << stripCursor(source);

    source = QLatin1String("int f@(char *s);");
    QTest::newRow("function-declaration-param_out-start")
        << source << stripCursor(source);

    source = QLatin1String("int f(char *s =@ 0);");
    QTest::newRow("function-declaration-param_out-end")
        << source << stripCursor(source);

    // Function definitions are handled by the same code as function
    // declarations, so just check a minimal example.
    QTest::newRow("function-definition-param_in-middle")
        << "int f(char @*s) {}"
        << "int f(char * s) {}";

    QTest::newRow("function-declaration-returntype_in-start")
        << "@char *f();"
        << "char * f();";

    QTest::newRow("function-declaration-returntype_in-end")
        << "char *f@();"
        << "char * f();";

    source = QLatin1String("char *f(@);");
    QTest::newRow("function-declaration-returntype_out-end")
        << source << stripCursor(source);

    QTest::newRow("function-definition-returntype_in-end")
        << "char *f@() {}"
        << "char * f() {}";

    QTest::newRow("function-definition-returntype_in-start")
        << "@char *f() {}"
        << "char * f() {}";

    source = QLatin1String("char *f(@) {}");
    QTest::newRow("function-definition-returntype_out-end")
        << source << stripCursor(source);

    source = QLatin1String("inline@ char *f() {}");
    QTest::newRow("function-definition-returntype-inline_out-start")
        << source << stripCursor(source);

    // Same code is also used for for other specifiers like virtual, inline, friend, ...
    QTest::newRow("function-definition-returntype-static-inline_in-start")
        << "static inline @char *f() {}"
        << "static inline char * f() {}";

    QTest::newRow("function-declaration-returntype-static-inline_in-start")
        << "static inline @char *f();"
        << "static inline char * f();";

    source = QLatin1String("static inline@ char *f() {}");
    QTest::newRow("function-definition-returntype-static-inline_out-start")
        << source << stripCursor(source);

    QTest::newRow("function-declaration-returntype-static-custom-type_in-start")
        << "static @CustomType *f();"
        << "static CustomType * f();";

    source = QLatin1String("static@ CustomType *f();");
    QTest::newRow("function-declaration-returntype-static-custom-type_out-start")
        << source << stripCursor(source);

    QTest::newRow("function-declaration-returntype-symbolvisibilityattributes_in-start")
        << "__attribute__((visibility(\"default\"))) @char *f();"
        << "__attribute__((visibility(\"default\"))) char * f();";

    source = QLatin1String("@__attribute__((visibility(\"default\"))) char *f();");
    QTest::newRow("function-declaration-returntype-symbolvisibilityattributes_out-start")
        << source << stripCursor(source);

    // The following two are not supported at the moment.
    source = QLatin1String("@char * __attribute__((visibility(\"default\"))) f();");
    QTest::newRow("function-declaration-returntype-symbolvisibilityattributes_out-start")
        << source << stripCursor(source);

    source = QLatin1String("@char * __attribute__((visibility(\"default\"))) f() {}");
    QTest::newRow("function-definition-returntype-symbolvisibilityattributes_out-start")
        << source << stripCursor(source);

    // NOTE: Since __declspec() is not parsed (but defined to nothing),
    // we can't detect it properly. Therefore, we fail on code with
    // FOO_EXPORT macros with __declspec() for pointer return types;

    QTest::newRow("typedef_in-start")
        << "typedef @char *s;"
        << "typedef char * s;";

    source = QLatin1String("@typedef char *s;");
    QTest::newRow("typedef_out-start")
        << source << stripCursor(source);

    QTest::newRow("static_in-start")
        << "static @char *s;"
        << "static char * s;";

    QTest::newRow("pointerToFunction_in-start")
        << "int (*bar)(@char *s);"
        << "int (*bar)(char * s);";

    source = QLatin1String("int (@*bar)();");
    QTest::newRow("pointerToFunction_in-start")
        << source << stripCursor(source);

    source = QLatin1String("int (@*bar)[] = 0;");
    QTest::newRow("pointerToArray_in-start")
        << source << stripCursor(source);

    //
    // Additional test cases that does not fit into the naming scheme
    //

    // The following case is a side effect of the reformatting. Though
    // the pointer type is according to the overview, the double space
    // before 'char' gets corrected.
    QTest::newRow("remove-extra-whitespace")
        << "@const  char * s = 0;"
        << "const char * s = 0;";

    // Nothing to pretty print since no pointer or references are involved.
    source = QLatin1String("@char  bla;"); // Two spaces to get sure nothing is reformatted.
    QTest::newRow("precondition-fail-no-pointer")
        << source << stripCursor(source);
}

void CppToolsPlugin::test_format_pointerdeclaration_in_controlflowstatements()
{
    QFETCH(QString, source);
    QFETCH(QString, reformattedSource);

    TestEnvironment env(source.toLatin1(), Document::ParseStatement);
    AST *ast = env.translationUnit->ast();
    QVERIFY(ast);

    env.applyFormatting(ast, PointerDeclarationFormatter::RespectCursor);

    QCOMPARE(env.textDocument->toPlainText(), reformattedSource);
}

void CppToolsPlugin::test_format_pointerdeclaration_in_controlflowstatements_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("reformattedSource");

    QString source;

    //
    // Same naming scheme as in test_format_pointerdeclaration_in_simpledeclarations_data()
    //

    QTest::newRow("if_in-start")
        << "if (@char *s = 0);"
        << "if (char * s = 0);";

    source = QLatin1String("if @(char *s = 0);");
    QTest::newRow("if_out-start")
        << source << stripCursor(source);

    QTest::newRow("if_in-end")
        << "if (char *s@ = 0);"
        << "if (char * s = 0);";

    source = QLatin1String("if (char *s @= 0);");
    QTest::newRow("if_out-end")
        << source << stripCursor(source);

    // if and for statements are handled by the same code, so just
    // check minimal examples for these
    QTest::newRow("while")
        << "while (@char *s = 0);"
        << "while (char * s = 0);";

    QTest::newRow("for")
        << "for (;@char *s = 0;);"
        << "for (;char * s = 0;);";

    // Should also work since it's a simple declaration
    // for (char *s = 0; true;);

    QTest::newRow("foreach_in-start")
        << "foreach (@char *s, list);"
        << "foreach (char * s, list);";

    source = QLatin1String("foreach @(char *s, list);");
    QTest::newRow("foreach-out-start")
        << source << stripCursor(source);

    QTest::newRow("foreach_in_end")
        << "foreach (const char *s@, list);"
        << "foreach (const char * s, list);";

    source = QLatin1String("foreach (const char *s,@ list);");
    QTest::newRow("foreach_out_end")
        << source << stripCursor(source);

    //
    // Additional test cases that does not fit into the naming scheme
    //

    source = QLatin1String("@if (char  s = 0);"); // Two spaces to get sure nothing is reformatted.
    QTest::newRow("precondition-fail-no-pointer") << source << stripCursor(source);
}

void CppToolsPlugin::test_format_pointerdeclaration_multiple_declarators()
{
    QFETCH(QString, source);
    QFETCH(QString, reformattedSource);

    TestEnvironment env(source.toLatin1(), Document::ParseDeclaration);
    AST *ast = env.translationUnit->ast();
    QVERIFY(ast);

    env.applyFormatting(ast, PointerDeclarationFormatter::RespectCursor);

    QCOMPARE(env.textDocument->toPlainText(), reformattedSource);
}

void CppToolsPlugin::test_format_pointerdeclaration_multiple_declarators_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("reformattedSource");

    QString source;

    QTest::newRow("function-declaration_in-start")
        << "char *s = 0, @*f(int i) = 0;"
        << "char *s = 0, * f(int i) = 0;";

    QTest::newRow("non-pointer-before_in-start")
        << "char c, @*t;"
        << "char c, * t;";

    QTest::newRow("pointer-before_in-start")
        << "char *s, @*t;"
        << "char *s, * t;";

    QTest::newRow("pointer-before_in-end")
        << "char *s, *t@;"
        << "char *s, * t;";

    source = QLatin1String("char *s,@ *t;");
    QTest::newRow("md1-out_start")
        << source << stripCursor(source);

    source = QLatin1String("char *s, *t;@");
    QTest::newRow("md1-out_end")
        << source << stripCursor(source);

    QTest::newRow("non-pointer-after_in-start")
        << "char c, @*t, d;"
        << "char c, * t, d;";

    QTest::newRow("pointer-after_in-start")
        << "char c, @*t, *d;"
        << "char c, * t, *d;";

    QTest::newRow("function-pointer_in-start")
        << "char *s, @*(*foo)(char *s) = 0;"
        << "char *s, *(*foo)(char * s) = 0;";
}

void CppToolsPlugin::test_format_pointerdeclaration_multiple_matches()
{
    QFETCH(QString, source);
    QFETCH(QString, reformattedSource);

    TestEnvironment env(source.toLatin1(), Document::ParseTranlationUnit);
    AST *ast = env.translationUnit->ast();
    QVERIFY(ast);

    env.applyFormatting(ast, PointerDeclarationFormatter::IgnoreCursor);

    QCOMPARE(env.textDocument->toPlainText(), reformattedSource);
}

void CppToolsPlugin::test_format_pointerdeclaration_multiple_matches_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("reformattedSource");

    QTest::newRow("rvalue-reference")
        << "int g(Bar&&c) {}"
        << "int g(Bar && c) {}";

    QTest::newRow("if2")
        << "int g() { if (char *s = 0) { char *t = 0; } }"
        << "int g() { if (char * s = 0) { char * t = 0; } }";

    QTest::newRow("if1")
        << "int g() { if (int i = 0) { char *t = 0; } }"
        << "int g() { if (int i = 0) { char * t = 0; } }";

    QTest::newRow("for1")
        << "int g() { for (char *s = 0; char *t = 0; s++); }"
        << "int g() { for (char * s = 0; char * t = 0; s++); }";

    QTest::newRow("for2")
        << "int g() { for (char *s = 0; char *t = 0; s++) { char *u = 0; } }"
        << "int g() { for (char * s = 0; char * t = 0; s++) { char * u = 0; } }";

    QTest::newRow("for3")
        << "int g() { for (char *s; char *t = 0; s++) { char *u = 0; } }"
        << "int g() { for (char * s; char * t = 0; s++) { char * u = 0; } }";

    QTest::newRow("multiple-declarators")
        << "const char c, *s, *(*foo)(char *s) = 0;"
        << "const char c, * s, *(*foo)(char * s) = 0;";

    QTest::newRow("complex")
        <<
            "int *foo(const int &b, int*, int *&rpi)\n"
            "{\n"
            "    int*pi = 0;\n"
            "    int*const*const cpcpi = &pi;\n"
            "    int*const*pcpi = &pi;\n"
            "    int**const cppi = &pi;\n"
            "\n"
            "    void (*foo)(char *s) = 0;\n"
            "    int (*bar)[] = 0;\n"
            "\n"
            "    char *s = 0, *f(int i) = 0;\n"
            "    const char c, *s, *(*foo)(char *s) = 0;"
            "\n"
            "    for (char *s; char *t = 0; s++) { char *u = 0; }"
            "\n"
            "    return 0;\n"
            "}\n"
        <<
           "int * foo(const int & b, int *, int *& rpi)\n"
           "{\n"
           "    int * pi = 0;\n"
           "    int * const * const cpcpi = &pi;\n"
           "    int * const * pcpi = &pi;\n"
           "    int ** const cppi = &pi;\n"
           "\n"
           "    void (*foo)(char * s) = 0;\n"
           "    int (*bar)[] = 0;\n"
           "\n"
           "    char * s = 0, * f(int i) = 0;\n"
           "    const char c, * s, *(*foo)(char * s) = 0;"
           "\n"
           "    for (char * s; char * t = 0; s++) { char * u = 0; }"
           "\n"
           "    return 0;\n"
           "}\n";
}
