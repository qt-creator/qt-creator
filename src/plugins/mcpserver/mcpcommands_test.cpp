// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpcommands_test.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <mcp/server/toolregistry.h>

#include <texteditor/texteditor.h>

#include <utils/filepath.h>
#include <utils/temporarydirectory.h>

#include <QJsonObject>
#include <QScopeGuard>
#include <QTest>
#include <QTextCursor>

using namespace Utils;

namespace Mcp::Internal {

// Invokes a registered tool and returns its structured content. A registry-level
// refusal (unknown tool, schema violation) lands in *error, so a test can tell
// it apart from an error the tool itself reported in "reason".
static QJsonObject callTool(const QString &name, const QJsonObject &arguments, QString *error)
{
    error->clear();
    const Result<Schema::CallToolResult> result = ToolRegistry::callToolForTests(
        name, Schema::CallToolRequestParams{}.arguments(arguments));
    if (!result) {
        *error = result.error();
        return {};
    }
    return result->structuredContentAsObject();
}

static TextEditor::TextEditorWidget *openText(const TemporaryDirectory &dir, const QByteArray &text)
{
    const FilePath filePath = dir.filePath("selection.txt");
    if (!filePath.writeFileContents(text))
        return nullptr;
    Core::IEditor *editor = Core::EditorManager::openEditor(filePath);
    return editor ? TextEditor::TextEditorWidget::fromEditor(editor) : nullptr;
}

class McpCommandsTest final : public QObject
{
    Q_OBJECT

private slots:
    void testSelectTextSpansWholeLinesByDefault();
    void testSelectTextTakesOneBasedColumns();
    void testSelectTextRejectsAnInvalidRange();
};

void McpCommandsTest::testSelectTextSpansWholeLinesByDefault()
{
    TemporaryDirectory dir("qtc-mcpcommands-XXXXXX");
    QVERIFY(dir.isValid());
    const QScopeGuard closeEditors([] { Core::EditorManager::closeAllEditors(false); });
    TextEditor::TextEditorWidget *widget
        = openText(dir, "alpha one\nbeta two\ngamma three\n");
    QVERIFY(widget);

    QString error;
    const QJsonObject result
        = callTool("editor_select_text", {{"start_line", 2}, {"end_line", 3}}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(result.value("reason").toString(), QString("ok"));

    // Without a column the range runs from the start of the first line to the
    // end of the last one, and the line separators are newlines rather than the
    // U+2029 QTextCursor::selectedText() reports.
    QCOMPARE(result.value("text").toString(), QString("beta two\ngamma three"));
    QVERIFY(!result.value("text").toString().contains(QChar::ParagraphSeparator));

    // The selection the tool exists for is the editor's own, not just the text.
    const QTextCursor cursor = widget->textCursor();
    QCOMPARE(cursor.selectionStart(), 10); // Behind "alpha one\n".
    QCOMPARE(cursor.selectionEnd(), 30);   // Behind "gamma three", before its newline.
}

void McpCommandsTest::testSelectTextTakesOneBasedColumns()
{
    TemporaryDirectory dir("qtc-mcpcommands-XXXXXX");
    QVERIFY(dir.isValid());
    const QScopeGuard closeEditors([] { Core::EditorManager::closeAllEditors(false); });
    QVERIFY(openText(dir, "alpha one\nbeta two\n"));

    QString error;
    const QJsonObject result = callTool(
        "editor_select_text",
        {{"start_line", 2}, {"start_column", 6}, {"end_line", 2}, {"end_column", 9}},
        &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(result.value("text").toString(), QString("two"));

    // A column past the end of the line selects up to that end.
    const QJsonObject clamped = callTool(
        "editor_select_text",
        {{"start_line", 1}, {"start_column", 7}, {"end_line", 1}, {"end_column", 99}},
        &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(clamped.value("text").toString(), QString("one"));
}

void McpCommandsTest::testSelectTextRejectsAnInvalidRange()
{
    TemporaryDirectory dir("qtc-mcpcommands-XXXXXX");
    QVERIFY(dir.isValid());
    const QScopeGuard closeEditors([] { Core::EditorManager::closeAllEditors(false); });
    QVERIFY(openText(dir, "alpha one\nbeta two\n"));

    QString error;
    const QJsonObject reversed
        = callTool("editor_select_text", {{"start_line", 2}, {"end_line", 1}}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(reversed.value("reason").toString(), QString("invalid_range"));

    const QJsonObject pastEnd
        = callTool("editor_select_text", {{"start_line", 1}, {"end_line", 99}}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(pastEnd.value("reason").toString(), QString("invalid_range"));
}

QObject *createMcpCommandsTest()
{
    return new McpCommandsTest;
}

} // namespace Mcp::Internal

#include "mcpcommands_test.moc"
