// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpcommands_test.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>

#include <mcp/server/toolregistry.h>

#include <texteditor/texteditor.h>

#include <utils/filepath.h>
#include <utils/multitextcursor.h>
#include <utils/temporarydirectory.h>

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDockWidget>
#include <QEventLoop>
#include <QGuiApplication>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QMap>
#include <QCursor>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QScopeGuard>
#include <QStandardItemModel>
#include <QTest>
#include <QTextCursor>
#include <QTimer>
#include <QTreeView>

#include <algorithm>

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
    void initTestCase();
    void cleanupTestCase();
    void testSelectTextSpansWholeLinesByDefault();
    void testSelectTextTakesOneBasedColumns();
    void testSelectTextRejectsAnInvalidRange();
    void testFindWidgetsReportsATextEditAsAnExcerpt();
    void testReadWidgetTextReturnsTheWholeDocument();
    void testCursorPositionIsOneBased();
    void testCursorPositionReportsTheSelectedRange();
    void testCursorPositionCutsALongLine();
    void testCursorPositionFollowsTheMainCursor();
    void testCursorPositionWithoutAnEditor();
    void testGetTextLeavesTheSelectionAlone();
    void testGetTextReadsAnEditorWithoutAFilePath();
    void testClickItemContextMenuKeepsAMultiSelection();
    void testClickItemShiftExtendsTheSelection();
    void testClickItemTogglesACheckBoxInItsOwnColumn();
    void testMouseEventOnAScrollAreaHitsTheGivenPoint();
    void testMouseEventOnAHeaderHitsTheHeader();
    void testMouseEventDoubleClickActivatesARow();
    void testMouseEventDoubleClickAnswersWhileItRunsALoop();
    void testShowCaption();
    void testClickGivesClickFocus();
    void testPressOnAnItemEditorKeepsItOpen();
    void testClickItemScrollsToACheckBoxColumn();
    void testListOpenReportsTheEditors();
    void testActivateMenuItemGoesThroughTheMenu();
    void testActivateMenuItemRefusesADisabledItem();
    void testClickWidgetReportsTheStateAfterTheClick();
    void testClickWidgetAnswersWhileTheClickRunsALoop();
    void testClickWidgetAnswersWhileTheHoldRunsALoop();
    void testClickWidgetDoesNotTakeAnEventLoopForABlock();
    void testClickWidgetDoesNotTakeAnOlderModalForABlock();
    void testClickWidgetAnswersWhileTheClickRunsAMenu();
    void testClickWidgetSurvivesTheClickedWidgetGoingAway();
    void testClickWidgetRefusesAWidgetGoneBeforeThePress();
    void testMouseEventRightClickRequestsAContextMenu();
    void testMouseEventAnswersWhileTheContextMenuRunsALoop();
    void testResizeDockResizesTheDockAroundAWidget();

private:
    QMap<QString, bool> m_uiToolStates;
};

// The ui_* tools are off by default, and this suite is the caller they exist
// for. The registry is process-wide, so note what each one was before.
void McpCommandsTest::initTestCase()
{
    for (const Schema::Tool &tool : ToolRegistry::registeredTools()) {
        const QString name = tool.name();
        if (!name.startsWith("ui_"))
            continue;
        m_uiToolStates.insert(name, ToolRegistry::isToolEnabled(name));
        ToolRegistry::enableTool(name, true);
    }
}

void McpCommandsTest::cleanupTestCase()
{
    for (auto it = m_uiToolStates.cbegin(); it != m_uiToolStates.cend(); ++it)
        ToolRegistry::enableTool(it.key(), it.value());
}

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

void McpCommandsTest::testReadWidgetTextReturnsTheWholeDocument()
{
    TemporaryDirectory dir("qtc-mcpcommands-XXXXXX");
    QVERIFY(dir.isValid());
    const QScopeGuard closeEditors([] { Core::EditorManager::closeAllEditors(false); });
    const QByteArray contents = QByteArray(600, 'x') + "\nthe end";
    TextEditor::TextEditorWidget *widget = openText(dir, contents);
    QVERIFY(widget);
    widget->setObjectName("mcpCommandsTestEdit");

    QString error;
    const QJsonObject whole = callTool(
        "ui_read_widget_text",
        {{"object_name", "mcpCommandsTestEdit"}, {"include_invisible", true}},
        &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(whole.value("length").toInt(), contents.size());
    QCOMPARE(whole.value("text").toString().size(), contents.size());
    QVERIFY(!whole.value("truncated").toBool());

    // The end of a log is what a caller watches, so it is reachable without
    // reading everything before it.
    const QJsonObject tail = callTool(
        "ui_read_widget_text",
        {{"object_name", "mcpCommandsTestEdit"},
         {"include_invisible", true},
         {"max_chars", 7},
         {"tail", true}},
        &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(tail.value("text").toString(), QString("the end"));
    QVERIFY(tail.value("truncated").toBool());
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

void McpCommandsTest::testGetTextLeavesTheSelectionAlone()
{
    TemporaryDirectory dir("qtc-mcpcommands-XXXXXX");
    QVERIFY(dir.isValid());
    const QScopeGuard closeEditors([] { Core::EditorManager::closeAllEditors(false); });
    TextEditor::TextEditorWidget *widget = openText(dir, "alpha one\nbeta two\ngamma three\n");
    QVERIFY(widget);

    QTextCursor cursor(widget->document());
    cursor.setPosition(11);
    cursor.setPosition(23, QTextCursor::KeepAnchor);
    widget->setTextCursor(cursor);

    QString error;
    const QJsonObject whole = callTool("editor_get_text", {}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(whole.value("reason").toString(), QString("ok"));
    QCOMPARE(whole.value("path").toString(), dir.filePath("selection.txt").toUserOutput());
    QCOMPARE(whole.value("text").toString(), QString("alpha one\nbeta two\ngamma three\n"));

    const QJsonObject range
        = callTool("editor_get_text", {{"start_line", 2}, {"end_line", 3}}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(range.value("text").toString(), QString("beta two\ngamma three"));

    QCOMPARE(widget->textCursor().anchor(), 11);
    QCOMPARE(widget->textCursor().position(), 23);
}

void McpCommandsTest::testGetTextReadsAnEditorWithoutAFilePath()
{
    const QScopeGuard closeEditors([] { Core::EditorManager::closeAllEditors(false); });
    QString title = "McpCommandsTest Scratch";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, "unsaved scratch\ncontent");
    QVERIFY(editor);
    QVERIFY(editor->document()->filePath().isEmpty());
    QCOMPARE(Core::EditorManager::currentEditor(), editor);

    QString error;
    const QJsonObject result = callTool("editor_get_text", {}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(result.value("reason").toString(), QString("ok"));
    QVERIFY(result.value("path").toString().isEmpty());
    QCOMPARE(result.value("display_name").toString(), editor->document()->displayName());
    QCOMPARE(result.value("text").toString(), QString("unsaved scratch\ncontent"));
}

void McpCommandsTest::testClickItemContextMenuKeepsAMultiSelection()
{
    QStandardItemModel model;
    for (const char *label : {"alpha", "beta", "gamma"})
        model.appendRow(new QStandardItem(QLatin1String(label)));

    QListView view;
    view.setObjectName("mcpCommandsTestList");
    view.setModel(&model);
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    view.resize(200, 200);
    view.show();
    const QScopeGuard hideView([&view] { view.hide(); });

    QItemSelectionModel *selection = view.selectionModel();
    for (int row = 0; row < model.rowCount(); ++row)
        selection->select(model.index(row, 0), QItemSelectionModel::Select);
    QCOMPARE(selection->selectedIndexes().size(), 3);

    QString error;
    const QJsonObject result = callTool(
        "ui_click_item",
        {{"object_name", "mcpCommandsTestList"}, {"item", "beta"}, {"context_menu", true}},
        &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(result.value("path").toString(), QString("beta"));

    // A real right press on an already selected row leaves the selection alone,
    // so asking that row for its context menu must not reduce it to the row.
    QCOMPARE(selection->selectedIndexes().size(), 3);
    QCOMPARE(view.currentIndex(), model.index(1, 0));
}

void McpCommandsTest::testClickItemShiftExtendsTheSelection()
{
    QStandardItemModel model;
    for (const char *label : {"alpha", "beta", "gamma", "delta"})
        model.appendRow(new QStandardItem(QLatin1String(label)));

    QListView view;
    view.setObjectName("mcpCommandsTestShiftList");
    view.setModel(&model);
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    view.resize(200, 200);
    view.show();
    const QScopeGuard hideView([&view] { view.hide(); });

    QString error;
    callTool("ui_click_item", {{"object_name", "mcpCommandsTestShiftList"}, {"item", "beta"}},
             &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    callTool(
        "ui_click_item",
        {{"object_name", "mcpCommandsTestShiftList"}, {"item", "delta"}, {"modifiers", "shift"}},
        &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    // The view reads the modifier state from QGuiApplication, not from the
    // mouse event, when it decides whether to keep the selection anchor. A
    // click that leaves that state alone selects the clicked row only.
    QList<int> rows;
    for (const QModelIndex &index : view.selectionModel()->selectedRows())
        rows.append(index.row());
    std::sort(rows.begin(), rows.end());
    QCOMPARE(rows, QList<int>({1, 2, 3}));

    // And the keys must not stay down for whatever runs next.
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);
}

// As in the plugin list, where "Name" and "Load" are separate columns.
void McpCommandsTest::testClickItemTogglesACheckBoxInItsOwnColumn()
{
    QStandardItemModel model;
    for (const char *label : {"alpha", "beta"}) {
        auto load = new QStandardItem;
        load->setCheckable(true);
        load->setCheckState(Qt::Unchecked);
        model.appendRow({new QStandardItem(QLatin1String(label)), load});
    }
    model.item(0, 1)->setCheckState(Qt::PartiallyChecked);

    QTreeView view;
    view.setObjectName("mcpCommandsTestCheckTree");
    view.setModel(&model);
    view.resize(300, 200);
    view.show();
    const QScopeGuard hideView([&view] { view.hide(); });

    QString error;
    const QJsonObject item = callTool(
        "ui_click_item",
        {{"object_name", "mcpCommandsTestCheckTree"}, {"item", "beta"}, {"check_box", true}},
        &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    QCOMPARE(model.item(1, 1)->checkState(), Qt::Checked);
    QCOMPARE(model.item(0, 1)->checkState(), Qt::PartiallyChecked);
    QCOMPARE(item.value("checked"), QJsonValue(true));

    const QJsonArray items
        = callTool("ui_find_items", {{"object_name", "mcpCommandsTestCheckTree"}}, &error)
              .value("items")
              .toArray();
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(items.size(), 2);
    QCOMPARE(items.at(0).toObject().value("check_state"), QJsonValue("partially_checked"));
    QVERIFY(!items.at(1).toObject().contains("check_state"));
}

// The coordinates are local to the view, not to the viewport below its header.
void McpCommandsTest::testMouseEventOnAScrollAreaHitsTheGivenPoint()
{
    QStandardItemModel model;
    for (const char *label : {"alpha", "beta", "gamma", "delta", "epsilon"})
        model.appendRow(new QStandardItem(QLatin1String(label)));

    QTreeView view;
    view.setObjectName("mcpCommandsTestMouseTree");
    view.setModel(&model);
    view.resize(300, 200);
    view.show();
    const QScopeGuard hideView([&view] { view.hide(); });

    const QRect rect = view.visualRect(model.index(1, 0));
    const QPoint point = view.viewport()->mapTo(&view, rect.center());
    QString error;
    for (const char *action : {"press", "release"}) {
        callTool("ui_mouse_event",
                 {{"object_name", "mcpCommandsTestMouseTree"},
                  {"action", QLatin1String(action)},
                  {"x", point.x()},
                  {"y", point.y()}},
                 &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
    }

    QCOMPARE(view.currentIndex().row(), 1);
}

void McpCommandsTest::testMouseEventOnAHeaderHitsTheHeader()
{
    QStandardItemModel model;
    for (const char *label : {"alpha", "beta"})
        model.appendRow(new QStandardItem(QLatin1String(label)));

    QTreeView view;
    view.setObjectName("mcpCommandsTestHeaderTree");
    view.setModel(&model);
    view.resize(300, 200);
    view.show();
    const QScopeGuard hideView([&view] { view.hide(); });
    view.setCurrentIndex(model.index(1, 0));
    view.header()->setSectionsClickable(true);
    int clicked = 0;
    connect(view.header(), &QHeaderView::sectionClicked, this, [&clicked] { ++clicked; });

    const QPoint point = view.header()->geometry().center();
    QString error;
    for (const char *action : {"press", "release"}) {
        callTool("ui_mouse_event",
                 {{"object_name", "mcpCommandsTestHeaderTree"},
                  {"action", QLatin1String(action)},
                  {"x", point.x()},
                  {"y", point.y()}},
                 &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
    }

    QCOMPARE(clicked, 1);
    QCOMPARE(view.currentIndex().row(), 1);
}

void McpCommandsTest::testMouseEventDoubleClickActivatesARow()
{
    QStandardItemModel model;
    for (const char *label : {"alpha", "beta", "gamma"})
        model.appendRow(new QStandardItem(QLatin1String(label)));

    QTreeView view;
    view.setObjectName("mcpCommandsTestDoubleClickTree");
    view.setEditTriggers(QAbstractItemView::NoEditTriggers);
    view.setModel(&model);
    view.resize(300, 200);
    view.show();
    const QScopeGuard hideView([&view] { view.hide(); });
    QList<int> doubleClicked;
    connect(&view, &QAbstractItemView::doubleClicked, this, [&doubleClicked](const QModelIndex &i) {
        doubleClicked.append(i.row());
    });

    const QPoint point = view.viewport()->mapTo(&view, view.visualRect(model.index(1, 0)).center());
    QString error;
    callTool("ui_mouse_event",
             {{"object_name", "mcpCommandsTestDoubleClickTree"},
              {"action", "dblclick"},
              {"x", point.x()},
              {"y", point.y()}},
             &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    QCOMPARE(doubleClicked, QList<int>{1});
}

void McpCommandsTest::testMouseEventDoubleClickAnswersWhileItRunsALoop()
{
    QStandardItemModel model;
    model.appendRow(new QStandardItem("alpha"));
    QTreeView view;
    view.setObjectName("mcpCommandsTestBlockingDoubleClickTree");
    view.setEditTriggers(QAbstractItemView::NoEditTriggers);
    view.setModel(&model);
    view.resize(300, 200);
    QDialog dialog;
    connect(&view, &QAbstractItemView::doubleClicked, &dialog, [&dialog] {
        // As in testClickWidgetAnswersWhileTheClickRunsALoop.
        QTimer guard;
        guard.setSingleShot(true);
        connect(&guard, &QTimer::timeout, &dialog, &QDialog::reject);
        guard.start(0);
        dialog.exec();
    });
    view.show();
    const QScopeGuard hideView([&view] { view.hide(); });

    const QPoint point = view.viewport()->mapTo(&view, view.visualRect(model.index(0, 0)).center());
    bool answeredInsideLoop = false;
    bool blocking = false;
    QString error;
    ToolRegistry::callToolForTests(
        "ui_mouse_event",
        Schema::CallToolRequestParams{}.arguments(
            QJsonObject{{"object_name", "mcpCommandsTestBlockingDoubleClickTree"},
                        {"action", "dblclick"},
                        {"x", point.x()},
                        {"y", point.y()}}),
        [&](const Result<Schema::CallToolResult> &answer) {
            answeredInsideLoop |= dialog.isVisible();
            if (answer)
                blocking |= answer->structuredContentAsObject().value("blocking").toBool();
            else
                error = answer.error();
            dialog.reject();
        });
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(answeredInsideLoop);
    QVERIFY(blocking);
}

void McpCommandsTest::testShowCaption()
{
    const auto caption = [] {
        return Core::ICore::mainWindow()->findChild<QLabel *>("mcpCaption");
    };
    QString error;
    const QJsonObject shown
        = callTool("ui_show_caption", {{"text", "McpCommandsTestCaption"}, {"seconds", 0}}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(shown.value("shown").toBool(), true);
    QVERIFY(caption());
    QCOMPARE(caption()->text(), QString("McpCommandsTestCaption"));

    // A caption that stays up survives the end of a timed one shown before it.
    QString laterError;
    QTimer::singleShot(0, [&laterError] {
        callTool("ui_show_caption",
                 {{"text", "McpCommandsTestLater"}, {"seconds", 0}},
                 &laterError);
    });
    callTool("ui_show_caption", {{"text", "McpCommandsTestCaption"}, {"seconds", 0.2}}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY2(laterError.isEmpty(), qPrintable(laterError));
    QVERIFY(caption());
    QCOMPARE(caption()->text(), QString("McpCommandsTestLater"));

    const QJsonObject removed = callTool("ui_show_caption", {{"text", ""}}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(removed.value("shown").toBool(), false);
    QVERIFY(!caption());
}

// QApplication gives click focus only for spontaneous presses.
void McpCommandsTest::testClickGivesClickFocus()
{
    QWidget window;
    auto first = new QLineEdit(&window);
    first->setObjectName("mcpCommandsTestFirstEdit");
    auto second = new QLineEdit("McpCommandsTestSecondEdit", &window);
    second->move(0, 40);
    window.show();
    const QScopeGuard hideWindow([&window] { window.hide(); });
    first->setFocus();
    QCOMPARE(window.focusWidget(), first);

    QString error;
    callTool("ui_click_widget", {{"text", "McpCommandsTestSecondEdit"}}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(window.focusWidget(), second);

    callTool("ui_mouse_event",
             {{"object_name", "mcpCommandsTestFirstEdit"}, {"action", "press"}, {"x", 2}, {"y", 2}},
             &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(window.focusWidget(), first);

    auto inner = new QWidget(second);
    inner->setObjectName("mcpCommandsTestInnerNoFocus");
    inner->resize(5, 5);
    inner->show();
    QCOMPARE(inner->focusPolicy(), Qt::NoFocus);
    for (const char *action : {"press", "release"}) {
        callTool("ui_mouse_event",
                 {{"object_name", "mcpCommandsTestInnerNoFocus"},
                  {"action", QLatin1String(action)},
                  {"x", 2},
                  {"y", 2}},
                 &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
    }
    QCOMPARE(window.focusWidget(), second);
}

// Focus follows the widget under the point, so the press does not close the editor.
void McpCommandsTest::testPressOnAnItemEditorKeepsItOpen()
{
    QStandardItemModel model;
    for (const char *label : {"alpha", "beta"})
        model.appendRow(new QStandardItem(QLatin1String(label)));

    QTreeView view;
    view.setObjectName("mcpCommandsTestEditorTree");
    view.setModel(&model);
    view.resize(300, 200);
    view.show();
    const QScopeGuard hideView([&view] { view.hide(); });
    QVERIFY(QTest::qWaitForWindowActive(&view));
    const QModelIndex index = model.index(1, 0);
    view.edit(index);
    QWidget *editor = view.indexWidget(index);
    QVERIFY(editor);
    QVERIFY(editor->hasFocus());

    const QPoint point = editor->mapTo(&view, editor->rect().center());
    QString error;
    for (const char *action : {"press", "release"}) {
        callTool("ui_mouse_event",
                 {{"object_name", "mcpCommandsTestEditorTree"},
                  {"action", QLatin1String(action)},
                  {"x", point.x()},
                  {"y", point.y()}},
                 &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
    }
    QCOMPARE(view.indexWidget(index), editor);
    QVERIFY(editor->hasFocus());
}

void McpCommandsTest::testClickItemScrollsToACheckBoxColumn()
{
    QStandardItemModel model;
    for (const char *label : {"alpha", "beta"}) {
        auto load = new QStandardItem;
        load->setCheckable(true);
        load->setCheckState(Qt::Unchecked);
        model.appendRow({new QStandardItem(QLatin1String(label)), load});
    }

    QTreeView view;
    view.setObjectName("mcpCommandsTestNarrowCheckTree");
    view.setModel(&model);
    view.header()->setStretchLastSection(false);
    view.header()->resizeSection(0, 400);
    view.resize(100, 200);
    view.show();
    const QScopeGuard hideView([&view] { view.hide(); });

    QString error;
    callTool("ui_click_item",
             {{"object_name", "mcpCommandsTestNarrowCheckTree"},
              {"item", "beta"},
              {"check_box", true}},
             &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(model.item(1, 1)->checkState(), Qt::Checked);
}

void McpCommandsTest::testListOpenReportsTheEditors()
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
    const QJsonArray editors = callTool("editor_list_open", {}, &error).value("editors").toArray();
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(editors.size(), 1);
    const QJsonObject editor = editors.at(0).toObject();
    QCOMPARE(editor.value("path").toString(), dir.filePath("selection.txt").toUserOutput());
    QCOMPARE(editor.value("editor_id").toString(),
             QString(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID));
    QCOMPARE(editor.value("mime_type").toString(), QString("text/plain"));
    QCOMPARE(editor.value("line").toInt(), 2);
    QCOMPARE(editor.value("column").toInt(), 6);
    QCOMPARE(editor.value("widget_class").toString(),
             QString::fromLatin1(widget->metaObject()->className()));
}

void McpCommandsTest::testActivateMenuItemGoesThroughTheMenu()
{
    // QMenu::exec() only holds the popup up on a platform that can grab the
    // keyboard. Under "offscreen" the menu closes before the posted key
    // arrives, and exec() then answers nullptr however the item was activated.
    if (QGuiApplication::platformName() == QLatin1String("offscreen"))
        QSKIP("The offscreen platform does not keep a popup menu up.");

    QMenu menu;
    QAction *action = menu.addAction("McpCommandsTestItem");
    int triggered = 0;
    connect(action, &QAction::triggered, this, [&triggered] { ++triggered; });

    QString error;
    QJsonObject result;
    // The tool has to run while the menu is up, so queue the call into the
    // event loop exec() starts. Hiding the menu when the call never got that
    // far keeps exec() from outliving the test.
    QMetaObject::invokeMethod(
        &menu,
        [&] {
            result = callTool("ui_activate_menu_item", {{"title", "McpCommandsTestItem"}}, &error);
            if (!error.isEmpty())
                menu.hide();
        },
        Qt::QueuedConnection);

    // Most context menus in the tree read what exec() handed back, so emitting
    // triggered() is not enough: the menu itself has to do the activating, and
    // only then does it report the action as the one that was chosen.
    QAction *chosen = menu.exec(QPoint(0, 0));
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(result.value("triggered").toBool());
    QCOMPARE(chosen, action);
    QCOMPARE(triggered, 1);
}

void McpCommandsTest::testActivateMenuItemRefusesADisabledItem()
{
    QMenu menu;
    QAction *action = menu.addAction("McpCommandsTestDisabledItem");
    action->setEnabled(false);
    int triggered = 0;
    connect(action, &QAction::triggered, this, [&triggered] { ++triggered; });
    menu.popup(QPoint(0, 0));
    const QScopeGuard hideMenu([&menu] { menu.hide(); });

    // Activating a disabled item does nothing at all, so answering "triggered"
    // would send a caller looking for an effect that cannot come. The refusal
    // is immediate and posts nothing, so there is no later event to wait for.
    QString error;
    callTool("ui_activate_menu_item", {{"title", "McpCommandsTestDisabledItem"}}, &error);
    // Naming the item keeps a refusal of the tool itself - which the registry
    // also reports as "disabled" - from passing for the refusal under test.
    QVERIFY2(error.contains("Menu item is disabled"), qPrintable(error));
    QCOMPARE(triggered, 0);
}

void McpCommandsTest::testClickWidgetReportsTheStateAfterTheClick()
{
    QPushButton button("McpCommandsTestToggle");
    button.setCheckable(true);
    button.show();
    const QScopeGuard hideButton([&button] { button.hide(); });

    QString error;
    const QJsonObject result
        = callTool("ui_click_widget", {{"text", "McpCommandsTestToggle"}}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(button.isChecked());
    QVERIFY(result.value("checked").toBool());
    QVERIFY(!result.contains("blocking"));
}

void McpCommandsTest::testClickWidgetAnswersWhileTheClickRunsALoop()
{
    QPushButton button("McpCommandsTestBlockingButton");
    QDialog dialog;
    connect(&button, &QPushButton::clicked, &dialog, [&dialog] {
        // Started after the tool's own zero timer, so it fires after it, and
        // closes the dialog if the tool does not. Not QTimer::singleShot(), which
        // posts a zero timeout as an event, and events go before timers.
        QTimer guard;
        guard.setSingleShot(true);
        connect(&guard, &QTimer::timeout, &dialog, &QDialog::reject);
        guard.start(0);
        dialog.exec();
    });
    button.show();
    const QScopeGuard hideButton([&button] { button.hide(); });

    bool answeredInsideLoop = false;
    QJsonObject result;
    QString error;
    ToolRegistry::callToolForTests(
        "ui_click_widget",
        Schema::CallToolRequestParams{}.arguments(
            QJsonObject{{"text", "McpCommandsTestBlockingButton"}}),
        [&](const Result<Schema::CallToolResult> &answer) {
            answeredInsideLoop = dialog.isVisible();
            if (answer)
                result = answer->structuredContentAsObject();
            else
                error = answer.error();
            dialog.reject();
        });
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(answeredInsideLoop);
    QVERIFY(result.value("blocking").toBool());
}

void McpCommandsTest::testClickWidgetAnswersWhileTheHoldRunsALoop()
{
    QString error;
    callTool("ui_set_demo_pace", {{"click_hold_ms", 1}}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    const QScopeGuard resetPace([] {
        QString error;
        callTool("ui_set_demo_pace", {{"click_hold_ms", 0}}, &error);
    });

    // As a QToolButton with a delayed popup does: the press starts a timer that
    // runs a menu, so the loop runs while the click is held.
    QPushButton button("McpCommandsTestHoldButton");
    QDialog dialog;
    QTimer popupTimer;
    popupTimer.setSingleShot(true);
    connect(&popupTimer, &QTimer::timeout, &dialog, [&dialog] {
        QTimer guard;
        guard.setSingleShot(true);
        connect(&guard, &QTimer::timeout, &dialog, &QDialog::reject);
        guard.start(0);
        dialog.exec();
    });
    connect(&button, &QPushButton::pressed, &popupTimer, [&popupTimer] { popupTimer.start(0); });
    button.show();
    const QScopeGuard hideButton([&button] { button.hide(); });

    bool answeredInsideLoop = false;
    QJsonObject result;
    ToolRegistry::callToolForTests(
        "ui_click_widget",
        Schema::CallToolRequestParams{}.arguments(
            QJsonObject{{"text", "McpCommandsTestHoldButton"}}),
        [&](const Result<Schema::CallToolResult> &answer) {
            answeredInsideLoop = dialog.isVisible();
            if (answer)
                result = answer->structuredContentAsObject();
            else
                error = answer.error();
            dialog.reject();
        });
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(answeredInsideLoop);
    QVERIFY(result.value("blocking").toBool());
}

void McpCommandsTest::testClickWidgetDoesNotTakeAnEventLoopForABlock()
{
    // What a slot does that waits for a process, or calls processEvents().
    QPushButton button("McpCommandsTestBusyButton");
    QEventLoop loop;
    connect(&button, &QPushButton::clicked, &loop, [&loop] {
        QTimer guard;
        guard.setSingleShot(true);
        connect(&guard, &QTimer::timeout, &loop, &QEventLoop::quit);
        guard.start(0);
        loop.exec();
    });
    button.show();
    const QScopeGuard hideButton([&button] { button.hide(); });

    bool answeredInsideLoop = true;
    QJsonObject result;
    QString error;
    ToolRegistry::callToolForTests(
        "ui_click_widget",
        Schema::CallToolRequestParams{}.arguments(
            QJsonObject{{"text", "McpCommandsTestBusyButton"}}),
        [&](const Result<Schema::CallToolResult> &answer) {
            answeredInsideLoop = loop.isRunning();
            if (answer)
                result = answer->structuredContentAsObject();
            else
                error = answer.error();
        });
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(!answeredInsideLoop);
    QVERIFY(!result.contains("blocking"));
}

void McpCommandsTest::testClickWidgetDoesNotTakeAnOlderModalForABlock()
{
    QDialog outer;
    outer.setWindowModality(Qt::ApplicationModal);
    outer.show();
    QDialog inner;
    inner.setWindowModality(Qt::ApplicationModal);
    QPushButton button("McpCommandsTestCloseInnerButton", &inner);
    QEventLoop loop;
    connect(&button, &QPushButton::clicked, &loop, [&inner, &loop] {
        inner.hide();
        QTimer guard;
        guard.setSingleShot(true);
        connect(&guard, &QTimer::timeout, &loop, &QEventLoop::quit);
        guard.start(0);
        loop.exec();
    });
    inner.show();
    const QScopeGuard hideDialogs([&inner, &outer] {
        inner.hide();
        outer.hide();
    });

    QString error;
    const QJsonObject result
        = callTool("ui_click_widget", {{"text", "McpCommandsTestCloseInnerButton"}}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(!result.contains("blocking"));
}

void McpCommandsTest::testClickWidgetAnswersWhileTheClickRunsAMenu()
{
    // A menu makes no modal widget active, so only the popup tells the block apart.
    if (QGuiApplication::platformName() == QLatin1String("offscreen"))
        QSKIP("The offscreen platform does not keep a popup menu up.");

    QPushButton button("McpCommandsTestMenuButton");
    QMenu menu;
    menu.addAction("McpCommandsTestMenuItem");
    connect(&button, &QPushButton::clicked, &menu, [&menu] {
        QTimer guard;
        guard.setSingleShot(true);
        connect(&guard, &QTimer::timeout, &menu, &QMenu::hide);
        guard.start(0);
        menu.exec(QPoint(0, 0));
    });
    button.show();
    const QScopeGuard hideButton([&button] { button.hide(); });

    bool answeredInsideLoop = false;
    QJsonObject result;
    QString error;
    ToolRegistry::callToolForTests(
        "ui_click_widget",
        Schema::CallToolRequestParams{}.arguments(
            QJsonObject{{"text", "McpCommandsTestMenuButton"}}),
        [&](const Result<Schema::CallToolResult> &answer) {
            answeredInsideLoop = menu.isVisible();
            if (answer)
                result = answer->structuredContentAsObject();
            else
                error = answer.error();
            menu.hide();
        });
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(answeredInsideLoop);
    QVERIFY(result.value("blocking").toBool());
}

void McpCommandsTest::testClickWidgetSurvivesTheClickedWidgetGoingAway()
{
    QString error;
    callTool("ui_set_demo_pace", {{"click_hold_ms", 1}}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    const QScopeGuard resetPace([] {
        QString error;
        callTool("ui_set_demo_pace", {{"click_hold_ms", 0}}, &error);
    });

    QPointer<QPushButton> button = new QPushButton("McpCommandsTestVanishingButton");
    QTimer deleter;
    deleter.setSingleShot(true);
    connect(&deleter, &QTimer::timeout, [&button] { delete button; });
    connect(button, &QPushButton::pressed, &deleter, [&deleter] { deleter.start(0); });
    button->show();
    const QScopeGuard deleteButton([&button] { delete button; });

    const QJsonObject result
        = callTool("ui_click_widget", {{"text", "McpCommandsTestVanishingButton"}}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(!button);
    QCOMPARE(result.value("text").toString(), QString("McpCommandsTestVanishingButton"));
    QVERIFY(result.value("destroyed").toBool());
    QVERIFY(!result.contains("blocking"));
}

void McpCommandsTest::testClickWidgetRefusesAWidgetGoneBeforeThePress()
{
    QString error;
    callTool("ui_set_demo_pace", {{"pointer_speed", 10000}}, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    const QScopeGuard resetPace([] {
        QString error;
        callTool("ui_set_demo_pace", {{"pointer_speed", 0}}, &error);
    });

    QPointer<QPushButton> button = new QPushButton("McpCommandsTestGoneButton");
    button->show();
    const QScopeGuard deleteButton([&button] { delete button; });
    // Far enough away that the glide waits at least one frame, where the timer fires.
    QCursor::setPos(button->mapToGlobal(button->rect().center()) + QPoint(200, 200));
    QTimer deleter;
    deleter.setSingleShot(true);
    connect(&deleter, &QTimer::timeout, [&button] { delete button; });
    deleter.start(0);

    callTool("ui_click_widget", {{"text", "McpCommandsTestGoneButton"}}, &error);
    QVERIFY(!button);
    QVERIFY2(error.contains("went away before it could be clicked"), qPrintable(error));
}

void McpCommandsTest::testMouseEventRightClickRequestsAContextMenu()
{
    QWidget widget;
    widget.setObjectName("McpCommandsTestMenuTarget");
    widget.setContextMenuPolicy(Qt::CustomContextMenu);
    QList<QPoint> requested;
    connect(&widget, &QWidget::customContextMenuRequested, &widget, [&requested](QPoint pos) {
        requested.append(pos);
    });
    widget.show();
    const QScopeGuard hideWidget([&widget] { widget.hide(); });

    const auto click = [](const QString &button) {
        QString error;
        for (const char *action : {"press", "release"}) {
            callTool("ui_mouse_event",
                     {{"object_name", "McpCommandsTestMenuTarget"},
                      {"action", action},
                      {"x", 5},
                      {"y", 7},
                      {"button", button}},
                     &error);
            if (!error.isEmpty())
                return error;
        }
        return error;
    };

    QString error = click("left");
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(requested.isEmpty());

    error = click("right");
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(requested, QList<QPoint>{QPoint(5, 7)});
}

void McpCommandsTest::testMouseEventAnswersWhileTheContextMenuRunsALoop()
{
    QWidget widget;
    widget.setObjectName("McpCommandsTestBlockingMenuTarget");
    widget.setContextMenuPolicy(Qt::CustomContextMenu);
    QDialog dialog;
    connect(&widget, &QWidget::customContextMenuRequested, &dialog, [&dialog] {
        // As in testClickWidgetAnswersWhileTheClickRunsALoop.
        QTimer guard;
        guard.setSingleShot(true);
        connect(&guard, &QTimer::timeout, &dialog, &QDialog::reject);
        guard.start(0);
        dialog.exec();
    });
    widget.show();
    const QScopeGuard hideWidget([&widget] { widget.hide(); });

    bool answeredInsideLoop = false;
    bool blocking = false;
    QString error;
    for (const char *action : {"press", "release"}) {
        ToolRegistry::callToolForTests(
            "ui_mouse_event",
            Schema::CallToolRequestParams{}.arguments(
                QJsonObject{{"object_name", "McpCommandsTestBlockingMenuTarget"},
                            {"action", action},
                            {"x", 5},
                            {"y", 7},
                            {"button", "right"}}),
            [&](const Result<Schema::CallToolResult> &answer) {
                answeredInsideLoop |= dialog.isVisible();
                if (answer)
                    blocking |= answer->structuredContentAsObject().value("blocking").toBool();
                else
                    error = answer.error();
                dialog.reject();
            });
        QVERIFY2(error.isEmpty(), qPrintable(error));
    }
    QVERIFY(answeredInsideLoop);
    QVERIFY(blocking);
}

// The query may name a widget inside the dock, as the debugger's views are.
void McpCommandsTest::testResizeDockResizesTheDockAroundAWidget()
{
    QMainWindow mainWindow;
    mainWindow.setCentralWidget(new QLabel("central"));
    auto dock = new QDockWidget("Dock", &mainWindow);
    auto inner = new QListView;
    inner->setObjectName("mcpCommandsTestDockedList");
    dock->setWidget(inner);
    mainWindow.addDockWidget(Qt::RightDockWidgetArea, dock);
    mainWindow.resize(800, 400);
    mainWindow.show();
    const QScopeGuard hideWindow([&mainWindow] { mainWindow.hide(); });
    QTRY_VERIFY(dock->isVisible());

    for (int width : {150, 400}) {
        QString error;
        const QJsonObject result = callTool(
            "ui_resize_dock",
            {{"object_name", "mcpCommandsTestDockedList"}, {"width", width}},
            &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QVERIFY2(!result.contains("reason"), qPrintable(result.value("reason").toString()));
        QCOMPARE(dock->width(), width);
        QCOMPARE(result.value("width").toInt(), width);
    }
}

QObject *createMcpCommandsTest()
{
    return new McpCommandsTest;
}

} // namespace Mcp::Internal

#include "mcpcommands_test.moc"
