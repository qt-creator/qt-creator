// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpcommands_test.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <mcp/server/toolregistry.h>

#include <texteditor/texteditor.h>

#include <utils/filepath.h>
#include <utils/multitextcursor.h>
#include <utils/temporarydirectory.h>

#include <QJsonArray>
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
    void testFindWidgetsReportsATextEditAsAnExcerpt();
    void testCursorPositionIsOneBased();
    void testCursorPositionReportsTheSelectedRange();
    void testCursorPositionCutsALongLine();
    void testCursorPositionFollowsTheMainCursor();
    void testCursorPositionWithoutAnEditor();
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

void McpCommandsTest::testFindWidgetsReportsATextEditAsAnExcerpt()
{
    TemporaryDirectory dir("qtc-mcpcommands-XXXXXX");
    QVERIFY(dir.isValid());
    const QScopeGuard closeEditors([] { Core::EditorManager::closeAllEditors(false); });
    // The ampersands are content, not accelerator markers, and the document is
    // longer than the excerpt the tool answers with.
    TextEditor::TextEditorWidget *widget
        = openText(dir, "cmake --build . && ctest\n" + QByteArray(600, 'x'));
    QVERIFY(widget);
    widget->setObjectName("mcpCommandsTestEdit");

    QString error;
    const QJsonObject result = callTool(
        "ui_find_widgets",
        {{"object_name", "mcpCommandsTestEdit"}, {"include_invisible", true}},
        &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(result.value("count").toInt(), 1);

    const QJsonObject found = result.value("widgets").toArray().first().toObject();
    const QString text = found.value("text").toString();
    QVERIFY(text.startsWith("cmake --build . && ctest"));
    QCOMPARE(text.size(), 400);
    QVERIFY(found.value("text_truncated").toBool());
}

void McpCommandsTest::testCursorPositionIsOneBased()
{
    TemporaryDirectory dir("qtc-mcpcommands-XXXXXX");
    QVERIFY(dir.isValid());
    const QScopeGuard closeEditors([] { Core::EditorManager::closeAllEditors(false); });
    TextEditor::TextEditorWidget *widget = openText(dir, "alpha one\nbeta two\ngamma three\n");
    QVERIFY(widget);

    QTextCursor cursor(widget->document());
    cursor.setPosition(15); // Behind "alpha one\nbeta ".
    widget->setTextCursor(cursor);

    QString error;
    const QJsonObject result = callTool("editor_get_cursor_position", {}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(result.value("reason").toString(), QString("ok"));

    QCOMPARE(result.value("path").toString(), dir.filePath("selection.txt").toUserOutput());
    QCOMPARE(result.value("line").toInt(), 2);
    QCOMPARE(result.value("column").toInt(), 6);
    QCOMPARE(result.value("line_text").toString(), QString("beta two"));
    QCOMPARE(result.value("cursor_count").toInt(), 1);
    QVERIFY(!result.value("has_selection").toBool());

    // A cursor without a selection reports no range at all, rather than a range
    // that reads as one character wide.
    QVERIFY(!result.contains("selection_start_line"));
    QVERIFY(!result.contains("selection_end_line"));

    // The coordinates are the ones editor_select_text takes.
    const QJsonObject selected = callTool(
        "editor_select_text",
        {{"start_line", result.value("line").toInt()},
         {"start_column", result.value("column").toInt()},
         {"end_line", result.value("line").toInt()},
         {"end_column", 9}},
        &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(selected.value("text").toString(), QString("two"));
}

void McpCommandsTest::testCursorPositionReportsTheSelectedRange()
{
    TemporaryDirectory dir("qtc-mcpcommands-XXXXXX");
    QVERIFY(dir.isValid());
    const QScopeGuard closeEditors([] { Core::EditorManager::closeAllEditors(false); });
    TextEditor::TextEditorWidget *widget = openText(dir, "alpha one\nbeta two\ngamma three\n");
    QVERIFY(widget);

    // Anchor behind "alpha one\nb", position behind "gamm": a selection that
    // starts and ends mid-line on two different lines.
    QTextCursor cursor(widget->document());
    cursor.setPosition(11);
    cursor.setPosition(23, QTextCursor::KeepAnchor);
    widget->setTextCursor(cursor);

    QString error;
    const QJsonObject result = callTool("editor_get_cursor_position", {}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(result.value("reason").toString(), QString("ok"));

    // The cursor itself sits at the end of the selection, not at its anchor.
    QCOMPARE(result.value("line").toInt(), 3);
    QCOMPARE(result.value("column").toInt(), 5);

    QVERIFY(result.value("has_selection").toBool());
    QCOMPARE(result.value("selection_start_line").toInt(), 2);
    QCOMPARE(result.value("selection_start_column").toInt(), 2);
    QCOMPARE(result.value("selection_end_line").toInt(), 3);
    QCOMPARE(result.value("selection_end_column").toInt(), 5);

    // Feeding the range back to editor_select_text reproduces the selection.
    const QJsonObject selected = callTool(
        "editor_select_text",
        {{"start_line", result.value("selection_start_line").toInt()},
         {"start_column", result.value("selection_start_column").toInt()},
         {"end_line", result.value("selection_end_line").toInt()},
         {"end_column", result.value("selection_end_column").toInt()}},
        &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(selected.value("text").toString(), QString("eta two\ngamm"));
}

void McpCommandsTest::testCursorPositionCutsALongLine()
{
    TemporaryDirectory dir("qtc-mcpcommands-XXXXXX");
    QVERIFY(dir.isValid());
    const QScopeGuard closeEditors([] { Core::EditorManager::closeAllEditors(false); });
    QVERIFY(openText(dir, QByteArray(600, 'x') + "\n"));

    QString error;
    const QJsonObject result = callTool("editor_get_cursor_position", {}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(result.value("line_text").toString().size(), 400);
    QVERIFY(result.value("line_text_truncated").toBool());
}

void McpCommandsTest::testCursorPositionFollowsTheMainCursor()
{
    TemporaryDirectory dir("qtc-mcpcommands-XXXXXX");
    QVERIFY(dir.isValid());
    const QScopeGuard closeEditors([] { Core::EditorManager::closeAllEditors(false); });
    TextEditor::TextEditorWidget *widget = openText(dir, "alpha one\nbeta two\ngamma three\n");
    QVERIFY(widget);

    // The cursor added last is the main one, and it is the one without a
    // selection here: the report follows it rather than the first cursor or
    // whichever cursor happens to have selected something.
    QTextCursor selecting(widget->document());
    selecting.setPosition(0);
    selecting.setPosition(5, QTextCursor::KeepAnchor);
    QTextCursor plain(widget->document());
    plain.setPosition(23); // Behind "alpha one\nbeta two\ngamm".
    widget->setMultiTextCursor(MultiTextCursor({selecting, plain}));

    QString error;
    const QJsonObject result = callTool("editor_get_cursor_position", {}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(result.value("cursor_count").toInt(), 2);
    QCOMPARE(result.value("line").toInt(), 3);
    QCOMPARE(result.value("column").toInt(), 5);
    QVERIFY(!result.value("has_selection").toBool());
}

void McpCommandsTest::testCursorPositionWithoutAnEditor()
{
    Core::EditorManager::closeAllEditors(false);
    QVERIFY(!Core::EditorManager::currentEditor());

    QString error;
    const QJsonObject result = callTool("editor_get_cursor_position", {}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(result.value("reason").toString(), QString("no_text_editor"));
    QVERIFY(!result.contains("line"));
}

QObject *createMcpCommandsTest()
{
    return new McpCommandsTest;
}

} // namespace Mcp::Internal

#include "mcpcommands_test.moc"
