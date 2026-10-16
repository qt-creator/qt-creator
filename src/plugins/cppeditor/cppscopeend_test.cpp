// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "cppscopeend_test.h"

#include "cppeditordocument.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <texteditor/displaysettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/temporarydirectory.h>

#include <QTest>
#include <QTextBlock>
#include <QTextCursor>

using namespace Core;
using namespace Utils;

namespace CppEditor::Internal {

class CppScopeEndTest final : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void testScopesNameTheirHeaders();
    void testInactiveBranchIsSkipped();
    void testRunsOfClosingBraces();
};

void CppScopeEndTest::cleanup()
{
    EditorManager::closeAllEditors(false);
}

static void enableScopeEnds(TextEditor::TextEditorWidget *widget)
{
    TextEditor::DisplaySettingsData settings = TextEditor::displaySettings().data();
    settings.m_displayScopeEnds = true;
    settings.m_scopeEndMinimumLines = 5;
    widget->setDisplaySettings(settings);
}

// The labels of all lines, by 1-based line number.
static QMap<int, QString> labels(const TextEditor::TextEditorWidget *widget)
{
    QMap<int, QString> result;
    for (QTextBlock block = widget->document()->begin(); block.isValid(); block = block.next()) {
        const QString label = widget->scopeEndLabel(block);
        if (!label.isEmpty())
            result.insert(block.blockNumber() + 1, label);
    }
    return result;
}

static QString body(int lines)
{
    QString result;
    for (int i = 0; i < lines; ++i)
        result += QString("    statement%1();\n").arg(i);
    return result;
}

void CppScopeEndTest::testScopesNameTheirHeaders()
{
    TemporaryDirectory tempDir("qtc-cpp-scopeend-XXXXXX");
    QVERIFY(tempDir.isValid());
    const FilePath filePath = tempDir.filePath("scopes.cpp");

    // Each scope is 6 lines long, so the minimum below only lets these through.
    QString text;
    int line = 1;
    QMap<int, QString> expected;
    const auto scope = [&](const QString &header, const QString &label,
                           const QString &close = "}") {
        text += header + "\n" + body(5) + close + "\n";
        line += int(header.count('\n')) + 1 + 5;
        expected.insert(line, label);
        line += int(close.count('\n')) + 1;
    };
    scope("void f(int a)\n{", "void f(int a)");
    scope("void Foo::bar(int a,\n          int b) const\n{", "void Foo::bar(int a, int b) const");
    scope("Foo::Foo()\n    : a(1)\n    , b(2)\n{", "Foo::Foo()");
    scope("Foo::Foo()\n    : a(1),\n      b(2)\n{", "Foo::Foo()");
    scope("Foo::Foo() : a(1), b(2) {", "Foo::Foo()");
    scope("Foo::Foo()\n    : a{1}\n    , b{2}\n{", "Foo::Foo()");
    scope("Foo::Foo()\n    : a{1},\n      b{2}\n{", "Foo::Foo()");
    scope("Foo::Foo(int a,\n         int b) noexcept\n    : a(a)\n{", "Foo::Foo(int a, int b) noexcept");
    scope("/*!\n    Why a parent?\n*/\nFoo::Foo(QObject *parent)\n    : QObject(parent)\n{",
          "Foo::Foo(QObject *parent)");
    scope("/*!\n    1) (see setOptions()):\n*/\nFoo::Foo()\n    : a(1)\n{", "Foo::Foo()");
    scope("Foo::Foo(QObject *parent) // why?\n    : QObject(parent)\n{",
          "Foo::Foo(QObject *parent) // why?");
    scope("Foo() : a(0) {}\nFoo(int a)\n    : a(a)\n{", "Foo(int a)");
    scope("Foo() : Foo(0) {}\nFoo(int a)\n    : a(a)\n{", "Foo(int a)");
    scope("Foo::Foo()\n    : Base<int>{}\n    , b(2)\n{", "Foo::Foo()");
    scope("class Foo : public Bar\n{", "class Foo : public Bar");
    scope("class Foo : public QObject,\n            public Bar\n{",
          "class Foo : public QObject, public Bar");
    scope("class Foo\n    : public QObject\n    , public Bar\n{",
          "class Foo : public QObject , public Bar");
    scope("if (useLong ? longCondition()\n            : shortCondition()) {",
          "if (useLong ? longCondition() : shortCondition())");
    scope("void f(int a\n       , int b)\n{", "void f(int a , int b)");
    scope("void g() {\n    if (a &&\n        b) {", "if (a && b)");
    text += "}\n";
    line += 1;
    scope("++i;\nif (c == ')' || text == \"]\") {", "if (c == ')' || text == \"]\")");
    scope("++i;\nif (text == R\"(a)b)\") {", "if (text == R\"(a)b)\")");
    scope("++i;\nif (a /* ) */) {", "if (a /* ) */)");
    scope("template<typename R>\n    requires std::ranges::view<R>\nclass View : public Base<View<R>>\n{",
          "template<typename R> requires std::ranges::view<R> class View : public Base<View<R>>",
          "};");
    scope("QHash<QString,\n      int> map {", "QHash<QString, int> map", "};");
    scope("template <typename T,\n          typename U> struct Pair {",
          "template <typename T, typename U> struct Pair", "};");
    scope("QList<QPair<int,\n                 int>> pairs {", "QList<QPair<int, int>> pairs", "};");
    scope("const auto handler = [this, document,\n                      line](int result) {",
          "const auto handler = [this, document, line](int result)", "};");
    scope("connect(object, // why?\n        &Object::changed,\n        this, [this] {",
          "connect(object, &Object::changed, this, [this]", "});");
    scope("// Can't fail.\n/* isn't virtual */ int f(int a /* can't be 0 */)\n{",
          "/* isn't virtual */ int f(int a /* can't be 0 */)");
    scope("const Group recipe {\n    parallel,\n    onGroupSetup([this] { return count < 0; }),\n"
          "    ProcessTask(onSetup, onDone),\n    onGroupDone([this] {",
          "onGroupDone([this]", "    })\n};");

    // An #endif in a comment ends nothing.
    text += "#ifdef Q_OS_MACOS\n" + body(3) + "/*\n#endif\n*/\n" + body(2) + "#endif\n";
    const int endifLine = int(text.count('\n'));
    QVERIFY(filePath.writeFileContents(text.toUtf8()));

    IEditor *editor = EditorManager::openEditor(filePath);
    QVERIFY(editor);
    auto baseEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditor::TextEditorWidget *widget = baseEditor->editorWidget();
    enableScopeEnds(widget);

    // The labels depend on what the highlighter has stored by now.
    QTRY_VERIFY(labels(widget).size() > expected.size());

    const QMap<int, QString> found = labels(widget);
    for (auto it = expected.cbegin(); it != expected.cend(); ++it)
        QCOMPARE(found.value(it.key()), it.value());
    QCOMPARE(found.value(endifLine), QString("#ifdef Q_OS_MACOS"));
    QCOMPARE(found.value(endifLine - 4), QString());

    // A comment after the #endif says what it ends already.
    QTextCursor cursor(widget->document()->findBlockByNumber(endifLine - 1));
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.insertText(" // Q_OS_MACOS");
    QCOMPARE(labels(widget).value(endifLine), QString());
}

void CppScopeEndTest::testInactiveBranchIsSkipped()
{
    TemporaryDirectory tempDir("qtc-cpp-scopeend-XXXXXX");
    QVERIFY(tempDir.isValid());
    const FilePath filePath = tempDir.filePath("inactive.cpp");

    // Both branches open a scope, and only one of them is closed.
    const QString text = "void f()\n"                          // 1
                         "{\n"
                         "#ifdef QTC_SCOPE_END_TEST_UNDEFINED\n"
                         "    if (useWinApi()) {\n"            // 4
                         "#else\n"
                         "    if (usePosix()) {\n"             // 6
                         "#endif\n"
                         + body(5)
                         + "    }\n"                           // 13
                         + body(5)
                         + "}\n";                              // 19
    QVERIFY(filePath.writeFileContents(text.toUtf8()));

    IEditor *editor = EditorManager::openEditor(filePath);
    QVERIFY(editor);
    auto baseEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditor::TextDocument *document = baseEditor->textDocument();
    TextEditor::TextEditorWidget *widget = baseEditor->editorWidget();
    enableScopeEnds(widget);

    const auto labelAt = [widget](int line) {
        return labels(widget).value(line);
    };
    QTRY_COMPARE(labelAt(19), QString("void f()"));
    QCOMPARE(labelAt(13), QString("if (usePosix())"));

    // The code model can change its mind without an edit, as when a header defines the macro.
    auto cppDocument = qobject_cast<CppEditorDocument *>(document);
    QVERIFY(cppDocument);
    const QTextBlock posixLine = document->document()->findBlockByNumber(5);
    const int lineEnd = posixLine.position() + posixLine.length() - 1;
    cppDocument->setIfdefedOutBlocks({TextEditor::BlockRange(posixLine.position(), lineEnd)});
    QTRY_COMPARE(labelAt(13), QString("if (useWinApi())"));
    QCOMPARE(labelAt(19), QString("void f()"));
}

void CppScopeEndTest::testRunsOfClosingBraces()
{
    TemporaryDirectory tempDir("qtc-cpp-scopeend-XXXXXX");
    QVERIFY(tempDir.isValid());
    const FilePath filePath = tempDir.filePath("runs.cpp");

    const QString text = "void f()\n"                                   // 1
                         "{\n"
                         "    for (int i = 0; i < n; ++i) {\n"
                         "        if (a) {\n"
                         + body(5)
                         + "}}}\n"                                      // 10
                         "void g()\n"
                         "{\n"
                         "    call(a, [this] {\n"                       // 13
                         "        if (b) {\n"
                         + body(5)
                         + "    }});\n"                                 // 20
                         "}\n";
    QVERIFY(filePath.writeFileContents(text.toUtf8()));

    IEditor *editor = EditorManager::openEditor(filePath);
    QVERIFY(editor);
    auto baseEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditor::TextEditorWidget *widget = baseEditor->editorWidget();
    enableScopeEnds(widget);

    const auto braceLabels = [widget](int line) {
        const QTextBlock block = widget->document()->findBlockByNumber(line - 1);
        QStringList result;
        for (int i = 0; i < block.text().size(); ++i) {
            if (block.text().at(i) == '}')
                result.append(widget->scopeEndLabel(block, i));
        }
        return result;
    };
    QTRY_COMPARE(labels(widget).value(10), QString("void f()"));
    QCOMPARE(braceLabels(10),
             QStringList({"if (a)", "for (int i = 0; i < n; ++i)", "void f()"}));
    QCOMPARE(labels(widget).value(20), QString("call(a, [this]"));
    QCOMPARE(braceLabels(20), QStringList({"if (b)", "call(a, [this]"}));
}

QObject *createCppScopeEndTest()
{
    return new CppScopeEndTest;
}

} // namespace CppEditor::Internal

#include "cppscopeend_test.moc"

#endif // WITH_TESTS
