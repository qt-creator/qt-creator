// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "texteditor_test.h"

#include "storagesettings.h"
#include "tabsettings.h"
#include "textdocument.h"
#include "texteditor.h"
#include "marginsettings.h"
#include "snippets/snippet.h"
#include "snippets/snippetparser.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/externaltoolmanager.h>
#include <coreplugin/find/basetextfind.h>
#include <coreplugin/idocument.h>

#include <utils/filepath.h>
#include <utils/multitextcursor.h>
#include <utils/temporarydirectory.h>

#include <QDropEvent>
#include <QMimeData>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>

#include <memory>
#include <optional>

namespace TextEditor::Internal {

static QString tabPolicyToString(TabSettingsData::TabPolicy policy)
{
    switch (policy) {
    case TabSettingsData::SpacesOnlyTabPolicy:
        return QLatin1String("spacesOnlyPolicy");
    case TabSettingsData::TabsOnlyTabPolicy:
        return QLatin1String("tabsOnlyPolicy");
    }
    return QString();
}

static QString continuationAlignBehaviorToString(TabSettingsData::ContinuationAlignBehavior behavior)
{
    switch (behavior) {
    case TabSettingsData::NoContinuationAlign:
        return QLatin1String("noContinuation");
    case TabSettingsData::ContinuationAlignWithSpaces:
        return QLatin1String("spacesContinuation");
    case TabSettingsData::ContinuationAlignWithIndent:
        return QLatin1String("indentContinuation");
    }
    return QString();
}

struct TabSettingsFlags
{
    TabSettingsData::TabPolicy policy;
    TabSettingsData::ContinuationAlignBehavior behavior;
};

using IsClean = std::function<bool (TabSettingsFlags)>;

static void generateTestRows(const QLatin1String &name, const QString &text, IsClean isClean)
{
    const QList<TabSettingsData::TabPolicy> allPolicies = {
        TabSettingsData::SpacesOnlyTabPolicy,
        TabSettingsData::TabsOnlyTabPolicy,
    };
    const QList<TabSettingsData::ContinuationAlignBehavior> allbehaviors = {
        TabSettingsData::NoContinuationAlign,
        TabSettingsData::ContinuationAlignWithSpaces,
        TabSettingsData::ContinuationAlignWithIndent
    };

    const QLatin1Char splitter('_');
    const int indentSize = 3;

    for (auto policy : allPolicies) {
        for (auto behavior : allbehaviors) {
            const QString tag = tabPolicyToString(policy) + splitter
                    + continuationAlignBehaviorToString(behavior) + splitter
                    + name;
            QTest::newRow(tag.toLatin1().data())
                    << policy
                    << behavior
                    << text
                    << indentSize
                    << isClean({policy, behavior});
        }
    }
}

class TextEditorTest final : public QObject
{
    Q_OBJECT

private slots:
    void testIndentationClean_data();
    void testIndentationClean();
    void testIndentUnindent_data();
    void testIndentUnindent();
    void testMakefileForcesTabPolicy();
    void testTextDocumentChanged();
    void testMoveLinesEndingInEmptyLine();
};

void TextEditorTest::testIndentationClean_data()
{
    QTest::addColumn<TabSettingsData::TabPolicy>("policy");
    QTest::addColumn<TabSettingsData::ContinuationAlignBehavior>("behavior");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("indentSize");
    QTest::addColumn<bool>("clean");

    generateTestRows(QLatin1String("emptyString"), QString::fromLatin1(""),
                     [](TabSettingsFlags) -> bool {
        return true;
    });

    generateTestRows(QLatin1String("spaceIndentation"), QString::fromLatin1("   f"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy != TabSettingsData::TabsOnlyTabPolicy;
    });

    generateTestRows(QLatin1String("spaceIndentationGuessTabs"), QString::fromLatin1("   f\n\tf"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy == TabSettingsData::SpacesOnlyTabPolicy;
    });

    generateTestRows(QLatin1String("tabIndentation"), QString::fromLatin1("\tf"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy == TabSettingsData::TabsOnlyTabPolicy;
    });

    generateTestRows(QLatin1String("tabIndentationGuessTabs"), QString::fromLatin1("\tf\n\tf"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy != TabSettingsData::SpacesOnlyTabPolicy;
    });

    generateTestRows(QLatin1String("doubleSpaceIndentation"), QString::fromLatin1("      f"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy != TabSettingsData::TabsOnlyTabPolicy
                && flags.behavior != TabSettingsData::NoContinuationAlign;
    });

    generateTestRows(QLatin1String("doubleTabIndentation"), QString::fromLatin1("\t\tf"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy == TabSettingsData::TabsOnlyTabPolicy
                && flags.behavior == TabSettingsData::ContinuationAlignWithIndent;
    });

    generateTestRows(QLatin1String("tabSpaceIndentation"), QString::fromLatin1("\t   f"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy == TabSettingsData::TabsOnlyTabPolicy
                && flags.behavior == TabSettingsData::ContinuationAlignWithSpaces;
    });
}

void TextEditorTest::testIndentationClean()
{
    // fetch test data
    QFETCH(TabSettingsData::TabPolicy, policy);
    QFETCH(TabSettingsData::ContinuationAlignBehavior, behavior);
    QFETCH(QString, text);
    QFETCH(int, indentSize);
    QFETCH(bool, clean);

    const TabSettingsData settings(policy, indentSize, indentSize, behavior);
    const QTextDocument doc(text);
    const QTextBlock block = doc.firstBlock();

    QCOMPARE(settings.isIndentationClean(block, indentSize), clean);
}

void TextEditorTest::testIndentUnindent_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("anchor");
    QTest::addColumn<int>("position");
    QTest::addColumn<TabSettingsData::TabPolicy>("policy");
    QTest::addColumn<int>("tabSize");
    QTest::addColumn<int>("indentSize");
    QTest::addColumn<bool>("doIndent");
    QTest::addColumn<QString>("expected");

    const auto Spaces = TabSettingsData::SpacesOnlyTabPolicy;
    const auto Tabs = TabSettingsData::TabsOnlyTabPolicy;

    // --- indent ---

    QTest::newRow("indent_emptyDocument")
        << QString() << 0 << 0 << Spaces << 8 << 4 << true << QString("    ");

    QTest::newRow("indent_singleLine_atStart_noSelection")
        << QString("foo") << 0 << 0 << Spaces << 8 << 4 << true << QString("    foo");

    QTest::newRow("indent_singleLine_fullSelection")
        << QString("foo") << 0 << 3 << Spaces << 8 << 4 << true << QString("    foo");

    QTest::newRow("indent_multiLine_selection")
        << QString("foo\nbar") << 0 << 7 << Spaces << 8 << 4 << true << QString("    foo\n    bar");

    QTest::newRow("indent_alreadyIndented_addsAnotherLevel")
        << QString("    foo") << 0 << 7 << Spaces << 8 << 4 << true << QString("        foo");

    QTest::newRow("indent_tabsOnlyPolicy")
        << QString("foo") << 0 << 3 << Tabs << 4 << 4 << true << QString("\tfoo");

    // --- unindent ---

    QTest::newRow("unindent_singleLine_atStart_noSelection")
        << QString("    foo") << 0 << 0 << Spaces << 8 << 4 << false << QString("foo");

    QTest::newRow("unindent_singleLine_fullSelection")
        << QString("    foo") << 0 << 7 << Spaces << 8 << 4 << false << QString("foo");

    QTest::newRow("unindent_multiLine_selection")
        << QString("    foo\n    bar") << 0 << 15 << Spaces << 8 << 4 << false
        << QString("foo\nbar");

    QTest::newRow("unindent_unindentedLine_isNoOp")
        << QString("foo") << 0 << 3 << Spaces << 8 << 4 << false << QString("foo");

    QTest::newRow("unindent_underAligned_stripsAllLeadingSpaces")
        << QString("  foo") << 0 << 5 << Spaces << 8 << 4 << false << QString("foo");

    QTest::newRow("unindent_overAligned_removesOneIndentStep")
        << QString("      foo") << 0 << 9 << Spaces << 8 << 4 << false << QString("  foo");

    QTest::newRow("unindent_tabIndented")
        << QString("\tfoo") << 0 << 4 << Tabs << 4 << 4 << false << QString("foo");

    // QTCREATORBUG-33260: unindent froze when m_indentSize > m_tabSize because
    // the old TabSettings::lineIndentPosition() helper could yield a negative
    // position, leaving the loop without forward progress.
    QTest::newRow("unindent_wideIndent_singleTab_QTCREATORBUG-33260")
        << QString("\trepeat") << 0 << 7 << Tabs << 8 << 16 << false << QString("repeat");

    // Three tabs with tabSize=8, indentSize=16 sit at column 24, which rounds down
    // to indent column 16; unindenting steps down to column 0 by removing two tabs.
    QTest::newRow("unindent_wideIndent_threeTabs_QTCREATORBUG-33260")
        << QString("\t\t\trepeat") << 0 << 9 << Tabs << 8 << 16 << false << QString("\trepeat");
}

void TextEditorTest::testIndentUnindent()
{
    QFETCH(QString, input);
    QFETCH(int, anchor);
    QFETCH(int, position);
    QFETCH(TabSettingsData::TabPolicy, policy);
    QFETCH(int, tabSize);
    QFETCH(int, indentSize);
    QFETCH(bool, doIndent);
    QFETCH(QString, expected);

    TextDocument doc;
    doc.setPlainText(input);

    TabSettingsData settings(policy, tabSize, indentSize, TabSettingsData::ContinuationAlignWithSpaces);
    settings.m_autoDetect = false;
    doc.setTabSettings(settings);

    QTextCursor cursor(doc.document());
    cursor.setPosition(anchor);
    cursor.setPosition(position, QTextCursor::KeepAnchor);
    Utils::MultiTextCursor multi({cursor});

    if (doIndent)
        doc.indent(multi);
    else
        doc.unindent(multi);

    QCOMPARE(doc.plainText(), expected);
}

void TextEditorTest::testMakefileForcesTabPolicy()
{
    // Makefiles are tab-sensitive, so the tab policy is forced to tabs
    // regardless of the configured code style - including for files that only
    // inherit text/x-makefile, such as debian/rules (QTCREATORBUG-3408).
    TabSettingsData spaces(TabSettingsData::SpacesOnlyTabPolicy, 8, 8,
                           TabSettingsData::ContinuationAlignWithSpaces);
    spaces.m_autoDetect = false;

    TextDocument makefile;
    makefile.setMimeType("text/x-makefile");
    makefile.setTabSettings(spaces);
    QCOMPARE(makefile.tabSettings().m_tabPolicy, TabSettingsData::TabsOnlyTabPolicy);

    // A non-makefile keeps the configured policy.
    TextDocument plain;
    plain.setMimeType("text/plain");
    plain.setTabSettings(spaces);
    QCOMPARE(plain.tabSettings().m_tabPolicy, TabSettingsData::SpacesOnlyTabPolicy);
}

void TextEditorTest::testTextDocumentChanged()
{
    TextEditorWidget widget;
    QSignalSpy signalSpy(&widget, &TextEditorWidget::textDocumentChanged);
    const TextDocumentPtr document(new TextDocument);

    widget.setTextDocument(document);

    QCOMPARE(signalSpy.count(), 1);
    QCOMPARE(widget.textDocument(), document.data());
}

// Regression test for QTCREATORBUG-34933.
void TextEditorTest::testMoveLinesEndingInEmptyLine()
{
    const QString input = "a\nb\n\nc\nd\ne";
    QString title = "move_lines.txt";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, input.toUtf8());
    QVERIFY(editor);
    const QScopeGuard cleanup([&] { Core::EditorManager::closeEditors({editor}, false); });
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *editorWidget = baseEditor->editorWidget();
    QVERIFY(editorWidget);

    // The lines "b" and "", selected from the start of "b" to the start of "c".
    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(2);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);

    editorWidget->moveLineDown();
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("a\nc\nb\n\nd\ne"));
    editorWidget->moveLineDown();
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("a\nc\nd\nb\n\ne"));
    editorWidget->moveLineUp();
    editorWidget->moveLineUp();
    QCOMPARE(editorWidget->textDocument()->plainText(), input);
}

QObject *createTextEditorTest()
{
    return new TextEditorTest;
}

// Regression test for QTCREATORBUG-24565.
class CleanWhitespaceTest final : public QObject
{
    Q_OBJECT

private slots:
    void testCleanWhitespace_data();
    void testCleanWhitespace();
};

void CleanWhitespaceTest::testCleanWhitespace_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("ignoreFileTypes");
    QTest::addColumn<bool>("skipTrailingWhitespace");
    QTest::addColumn<int>("tabPolicy");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<bool>("modified"); // whether cleaning is expected to change the document

    const QString tabBlankTab = "\t \t";  // trailing whitespace, as in the Squish test
    const QString tripleTab = "\t\t\t";   // whitespace-only line, as in the Squish test
    const QString defaultIgnore = "*.md, *.MD, Makefile";
    const int tabsOnly = TabSettingsData::TabsOnlyTabPolicy;
    const int spacesOnly = TabSettingsData::SpacesOnlyTabPolicy;

    // A regular source file: trailing whitespace is stripped and the
    // whitespace-only line is emptied (the QTCREATORBUG-24565 scenario).
    QTest::newRow("regularFile")
        << "main.cpp" << defaultIgnore << true << tabsOnly
        << ("foo" + tabBlankTab + "\n" + tripleTab + "\nbar\n")
        << QString("foo\n\nbar\n") << true;

    // A file matching the ignore list is left completely untouched: neither
    // trailing whitespace nor indentation (here the whitespace-only line) is
    // cleaned. "Skip clean whitespace for file types" skips all whitespace
    // cleanup, not just trailing (QTCREATORBUG-32894).
    QTest::newRow("ignoredExtension")
        << "notes.md" << defaultIgnore << true << tabsOnly
        << ("foo" + tabBlankTab + "\n" + tripleTab + "\nbar\n")
        << ("foo" + tabBlankTab + "\n" + tripleTab + "\nbar\n") << false;

    // The ignore list also matches plain file names without an extension. With
    // nothing left to clean, the document stays unmodified (Squish checked this
    // via the editor's "*" modification marker).
    QTest::newRow("ignoredName")
        << "Makefile" << defaultIgnore << true << tabsOnly
        << ("foo" + tabBlankTab + "\n")
        << ("foo" + tabBlankTab + "\n") << false;

    // A Makefile (on the ignore list) keeps its mandatory tab indentation even
    // under a Spaces-Only policy that would otherwise rewrite it to spaces
    // (QTCREATORBUG-32894).
    QTest::newRow("makefileTabIndentIsKept")
        << "Makefile" << defaultIgnore << true << spacesOnly
        << QString("all:\n\tgcc foo.c\n")
        << QString("all:\n\tgcc foo.c\n") << false;

    // A regular file under a Spaces-Only policy: tab indentation IS rewritten to
    // spaces (the counterpart to the Makefile case above).
    QTest::newRow("regularFileTabIndentConverted")
        << "main.cpp" << defaultIgnore << true << spacesOnly
        << QString("void f()\n{\n\tint x;\n}\n")
        << QString("void f()\n{\n    int x;\n}\n") << true;

    // With "skip trailing whitespace" disabled, trailing whitespace is trimmed
    // even for files on the ignore list (the counter-intuitive sense of the flag).
    QTest::newRow("alwaysTrimOverridesIgnoreList")
        << "notes.md" << defaultIgnore << false << tabsOnly
        << ("foo" + tabBlankTab + "\n")
        << QString("foo\n") << true;

    // With a Tabs-Only policy, a tab-indented code line must keep its tab and
    // must not be rewritten to spaces (QTCREATORBUG-33670).
    QTest::newRow("tabIndentIsKept")
        << "main.cpp" << defaultIgnore << true << tabsOnly
        << QString("if (x)\n\tfoo();\n")
        << QString("if (x)\n\tfoo();\n") << false;

    // Nested tab indentation stays as tabs, not tab-plus-spaces
    // (QTCREATORBUG-33670).
    QTest::newRow("nestedTabIndentIsKept")
        << "main.cpp" << defaultIgnore << true << tabsOnly
        << QString("if (x)\n\tif (y)\n\t\tfoo();\n")
        << QString("if (x)\n\tif (y)\n\t\tfoo();\n") << false;
}

void CleanWhitespaceTest::testCleanWhitespace()
{
    QFETCH(QString, fileName);
    QFETCH(QString, ignoreFileTypes);
    QFETCH(bool, skipTrailingWhitespace);
    QFETCH(int, tabPolicy);
    QFETCH(QString, input);
    QFETCH(QString, expected);
    QFETCH(bool, modified);

    TextDocument doc;
    doc.setFilePath(Utils::FilePath::fromString(fileName));

    StorageSettingsData settings;
    settings.m_ignoreFileTypes = ignoreFileTypes;
    settings.m_skipTrailingWhitespace = skipTrailingWhitespace;
    settings.m_cleanWhitespace = true;
    settings.m_inEntireDocument = true;
    settings.m_cleanIndentation = true;
    settings.m_addFinalNewLine = false;
    doc.setStorageSettings(settings);

    // Use a deterministic tab configuration instead of autodetection.
    TabSettingsData tabSettings(TabSettingsData::TabPolicy(tabPolicy), 4, 4,
                                TabSettingsData::ContinuationAlignWithSpaces);
    tabSettings.m_autoDetect = false;
    doc.setTabSettings(tabSettings);

    doc.setPlainText(input);
    doc.document()->setModified(false);

    QTextCursor cursor(doc.document());
    doc.cleanWhitespace(cursor);

    QCOMPARE(doc.plainText(), expected);
    QCOMPARE(doc.document()->isModified(), modified);
}

QObject *createCleanWhitespaceTest()
{
    return new CleanWhitespaceTest;
}

class SortLinesTest final : public QObject
{
    Q_OBJECT

private slots:
    void testSortLines();
};

void SortLinesTest::testSortLines()
{
    const QString input = "bbb\n\n aa\nAAA\naaa\n1A\n1a\naa\n!\n_\na";
    const QString expected = "\n aa\n!\n1A\n1a\nAAA\n_\na\naa\naaa\nbbb";

    QString title = "unsorted.txt";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, input.toUtf8());
    QVERIFY(editor);
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *editorWidget = baseEditor->editorWidget();
    QVERIFY(editorWidget);

    editorWidget->selectAll();
    editorWidget->sortLines();

    QCOMPARE(editorWidget->textDocument()->plainText(), expected);

    Core::EditorManager::closeEditors({editor}, false);
}

QObject *createSortLinesTest()
{
    return new SortLinesTest;
}

// Regression test for QTCREATORBUG-5757.
class SelectAllTest final : public QObject
{
    Q_OBJECT

private slots:
    void testCursorAfterSelectAll_data();
    void testCursorAfterSelectAll();
};

void SelectAllTest::testCursorAfterSelectAll_data()
{
    QTest::addColumn<int>("key");
    QTest::addColumn<int>("expected"); // 0 = at start, 1 = at end, 2 = only collapsed

    QTest::newRow("Left collapses to start") << int(Qt::Key_Left) << 0;
    QTest::newRow("Right collapses to end") << int(Qt::Key_Right) << 1;
    QTest::newRow("Down stays at end") << int(Qt::Key_Down) << 1;
    QTest::newRow("Up collapses") << int(Qt::Key_Up) << 2;
}

void SelectAllTest::testCursorAfterSelectAll()
{
    QFETCH(int, key);
    QFETCH(int, expected);

    const QString content = "first line\nsecond line\nthird line";
    QString title = "select_all.txt";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, content.toUtf8());
    QVERIFY(editor);
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *editorWidget = baseEditor->editorWidget();
    QVERIFY(editorWidget);

    const int size = editorWidget->textDocument()->plainText().length();

    editorWidget->selectAll();
    QTextCursor cursor = editorWidget->textCursor();
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectionStart(), 0);
    QCOMPARE(cursor.selectionEnd(), size);
    QCOMPARE(cursor.position(), size);

    QTest::keyClick(editorWidget, Qt::Key(key));

    cursor = editorWidget->textCursor();
    QVERIFY(!cursor.hasSelection());
    switch (expected) {
    case 0:
        QCOMPARE(cursor.position(), 0);
        break;
    case 1:
        QCOMPARE(cursor.position(), size);
        break;
    case 2:
        QCOMPARE(cursor.selectionStart(), cursor.selectionEnd());
        break;
    }

    Core::EditorManager::closeEditors({editor}, false);
}

QObject *createSelectAllTest()
{
    return new SelectAllTest;
}

class UndoCursorTest final : public QObject
{
    Q_OBJECT

private slots:
    void testSelectionRestoredOnUndo();
    void testMultiCursorRestoredOnUndo();
    void testMultiCursorTypingUndo();
    void testSelectionRestoredOnRedo();
    void testSelectAllRestoredOnUndo();
    void testEditAfterUndo();
    void testDropUndo();
    void testJoinedMoveLineUndo();
    void testJoinedIndentUndo();
    void testUndoRedoInDuplicate();
    void testUndoInOtherView();
    void testUndoOfEditInOtherEditor();
    void testUndoOfEditWhileNonTextEditorIsCurrent();
    void testUndoInEmbeddedWidget();
    void testUndoInFocusedWidget();

private:
    TextEditorWidget *openEditor(const QByteArray &contents);

    Core::IEditor *m_editor = nullptr;
};

TextEditorWidget *UndoCursorTest::openEditor(const QByteArray &contents)
{
    QString title = "undo_cursor.txt";
    m_editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, contents);
    if (!m_editor)
        return nullptr;
    auto baseEditor = qobject_cast<BaseTextEditor *>(m_editor);
    return baseEditor ? baseEditor->editorWidget() : nullptr;
}

void UndoCursorTest::testSelectionRestoredOnUndo()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(0);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);

    QTest::keyClick(editorWidget, Qt::Key_X);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("x line\nsecond line"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    cursor = editorWidget->textCursor();
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.anchor(), 0);
    QCOMPARE(cursor.position(), 5);
}

void UndoCursorTest::testMultiCursorRestoredOnUndo()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    QTextDocument *document = editorWidget->document();
    QTextCursor first(document);
    first.setPosition(0);
    first.setPosition(5, QTextCursor::KeepAnchor);
    QTextCursor second(document);
    second.setPosition(11);
    second.setPosition(17, QTextCursor::KeepAnchor);
    editorWidget->setMultiTextCursor(Utils::MultiTextCursor({first, second}));

    QTest::keyClick(editorWidget, Qt::Key_X);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("x line\nx line"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    const QList<QTextCursor> cursors = editorWidget->multiTextCursor().cursors();
    QCOMPARE(cursors.size(), 2);
    QCOMPARE(cursors.at(0).anchor(), 0);
    QCOMPARE(cursors.at(0).position(), 5);
    QCOMPARE(cursors.at(1).anchor(), 11);
    QCOMPARE(cursors.at(1).position(), 17);
}

void UndoCursorTest::testMultiCursorTypingUndo()
{
    const QString contents = "a\nb";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    // A cursor at the end of each line.
    QTextDocument *document = editorWidget->document();
    QTextCursor first(document);
    first.setPosition(1);
    QTextCursor second(document);
    second.setPosition(3);
    editorWidget->setMultiTextCursor(Utils::MultiTextCursor({first, second}));

    // Typing in a row is undone in one step, as with a single cursor.
    QTest::keyClicks(editorWidget, "xyz");
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("axyz\nbxyz"));
    editorWidget->undo();
    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    QCOMPARE(editorWidget->multiTextCursor().cursorCount(), 2);

    // Moving the cursors ends the step.
    QTest::keyClicks(editorWidget, "xy");
    QTest::keyClick(editorWidget, Qt::Key_Left);
    QTest::keyClicks(editorWidget, "z");
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("axzy\nbxzy"));
    editorWidget->undo();
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("axy\nbxy"));
    editorWidget->undo();
    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
}

void UndoCursorTest::testSelectionRestoredOnRedo()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(0);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);

    QTest::keyClick(editorWidget, Qt::Key_X);
    const QString edited = editorWidget->textDocument()->plainText();

    // the selection the redo has to bring back is not the one around the edit
    cursor = editorWidget->textCursor();
    cursor.setPosition(2);
    cursor.setPosition(6, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);

    editorWidget->undo();
    editorWidget->redo();

    QCOMPARE(editorWidget->textDocument()->plainText(), edited);
    cursor = editorWidget->textCursor();
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.anchor(), 2);
    QCOMPARE(cursor.position(), 6);
}

void UndoCursorTest::testSelectAllRestoredOnUndo()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    editorWidget->moveCursor(QTextCursor::End);
    editorWidget->selectAll();

    QTest::keyClick(editorWidget, Qt::Key_X);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("x"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    const QTextCursor cursor = editorWidget->textCursor();
    QCOMPARE(cursor.selectionStart(), 0);
    QCOMPARE(cursor.selectionEnd(), int(contents.size()));
}

void UndoCursorTest::testEditAfterUndo()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    editorWidget->moveCursor(QTextCursor::Start);
    QTest::keyClick(editorWidget, Qt::Key_A);
    editorWidget->undo();
    QCOMPARE(editorWidget->textDocument()->plainText(), contents);

    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(3);
    cursor.setPosition(7, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);
    QTest::keyClick(editorWidget, Qt::Key_B);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("firbine\nsecond line"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    cursor = editorWidget->textCursor();
    QCOMPARE(cursor.anchor(), 3);
    QCOMPARE(cursor.position(), 7);
}

void UndoCursorTest::testDropUndo()
{
    const QString contents = "foo bar baz";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(0);
    cursor.setPosition(3, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);

    QTextCursor end(editorWidget->document());
    end.movePosition(QTextCursor::End);
    QMimeData mimeData;
    mimeData.setText("foo");
    const QPoint dropPosition = editorWidget->cursorRect(end).center();
    // A drop goes to the widget that accepted the drag.
    QDragEnterEvent dragEnter(dropPosition, Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(editorWidget->viewport(), &dragEnter);
    QVERIFY(dragEnter.isAccepted());
    QDropEvent drop(dropPosition, Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(editorWidget->viewport(), &drop);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("foo bar bazfoo"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    cursor = editorWidget->textCursor();
    QCOMPARE(cursor.anchor(), 0);
    QCOMPARE(cursor.position(), 3);
}

void UndoCursorTest::testJoinedMoveLineUndo()
{
    TextEditorWidget *editorWidget = openEditor("a\nb\nc");
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(0);
    cursor.setPosition(1, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);
    QTest::keyClick(editorWidget, Qt::Key_X);

    cursor.setPosition(4);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);
    editorWidget->moveLineUp();
    editorWidget->moveLineUp();
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("c\nx\nb\n"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), QString("x\nb\nc"));
    cursor = editorWidget->textCursor();
    QCOMPARE(cursor.anchor(), 4);
    QCOMPARE(cursor.position(), 5);

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), QString("a\nb\nc"));
    cursor = editorWidget->textCursor();
    QCOMPARE(cursor.anchor(), 0);
    QCOMPARE(cursor.position(), 1);
}

void UndoCursorTest::testJoinedIndentUndo()
{
    TextEditorWidget *editorWidget = openEditor("a\nb\nc");
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    editorWidget->moveCursor(QTextCursor::End);
    QTest::keyClick(editorWidget, Qt::Key_X);
    const QString edited = editorWidget->textDocument()->plainText();
    QCOMPARE(edited, QString("a\nb\ncx"));

    QTextDocument *document = editorWidget->document();
    QTextCursor first(document);
    first.setPosition(0);
    first.setPosition(1, QTextCursor::KeepAnchor);
    QTextCursor second(document);
    second.setPosition(2);
    second.setPosition(3, QTextCursor::KeepAnchor);
    editorWidget->setMultiTextCursor(Utils::MultiTextCursor({first, second}));
    editorWidget->indent();
    QVERIFY(editorWidget->textDocument()->plainText() != edited);

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), edited);
    const QList<QTextCursor> cursors = editorWidget->multiTextCursor().cursors();
    QCOMPARE(cursors.size(), 2);
    QCOMPARE(cursors.at(0).anchor(), 0);
    QCOMPARE(cursors.at(0).position(), 1);
    QCOMPARE(cursors.at(1).anchor(), 2);
    QCOMPARE(cursors.at(1).position(), 3);

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), QString("a\nb\nc"));
    QVERIFY(!editorWidget->multiTextCursor().hasMultipleCursors());
    const QTextCursor cursor = editorWidget->textCursor();
    QVERIFY(!cursor.hasSelection());
    QCOMPARE(cursor.position(), 5);
}

void UndoCursorTest::testUndoRedoInDuplicate()
{
    QVERIFY(!Core::EditorManager::currentEditor());
    const QString contents = "first line\nsecond line";
    TextEditorWidget editorWidget;
    editorWidget.setTextDocument(TextDocumentPtr(new TextDocument));
    QVERIFY(editorWidget.textDocument()->setPlainText(contents));

    QTextCursor cursor = editorWidget.textCursor();
    cursor.setPosition(0);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget.setTextCursor(cursor);
    QTest::keyClick(&editorWidget, Qt::Key_X);
    const QString firstEdit = editorWidget.textDocument()->plainText();
    QCOMPARE(firstEdit, QString("x line\nsecond line"));

    TextEditorWidget duplicate;
    duplicate.setTextDocument(editorWidget.textDocumentPtr());
    TextEditorWidget *duplicateWidget = &duplicate;

    cursor = duplicateWidget->textCursor();
    cursor.setPosition(7);
    cursor.setPosition(13, QTextCursor::KeepAnchor);
    duplicateWidget->setTextCursor(cursor);
    QTest::keyClick(duplicateWidget, Qt::Key_Y);
    const QString secondEdit = duplicateWidget->textDocument()->plainText();
    QCOMPARE(secondEdit, QString("x line\ny line"));

    duplicateWidget->undo();
    duplicateWidget->undo();
    QCOMPARE(duplicateWidget->textDocument()->plainText(), contents);

    duplicateWidget->redo();

    QCOMPARE(duplicateWidget->textDocument()->plainText(), firstEdit);
    cursor = duplicateWidget->textCursor();
    QCOMPARE(cursor.anchor(), 7);
    QCOMPARE(cursor.position(), 13);

    duplicateWidget->redo();

    QCOMPARE(duplicateWidget->textDocument()->plainText(), secondEdit);
    cursor = duplicateWidget->textCursor();
    QVERIFY(!cursor.hasSelection());
    QCOMPARE(cursor.position(), 8);
}

void UndoCursorTest::testUndoInOtherView()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });
    QCOMPARE(TextEditorWidget::currentTextEditorWidget(), editorWidget);

    const std::unique_ptr<BaseTextEditor> duplicate(
        qobject_cast<BaseTextEditor *>(m_editor)->duplicate());
    QVERIFY(duplicate);
    TextEditorWidget *duplicateWidget = duplicate->editorWidget();
    QTextCursor cursor = duplicateWidget->textCursor();
    cursor.setPosition(15);
    duplicateWidget->setTextCursor(cursor);

    cursor = editorWidget->textCursor();
    cursor.setPosition(0);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);
    QTest::keyClick(editorWidget, Qt::Key_X);

    duplicateWidget->undo();

    // The undo in the other view moves its cursor to the undone edit.
    QCOMPARE(duplicateWidget->textDocument()->plainText(), contents);
    cursor = duplicateWidget->textCursor();
    QVERIFY(!cursor.hasSelection());
    QCOMPARE(cursor.position(), 5);
}

void UndoCursorTest::testUndoOfEditInOtherEditor()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *otherWidget = openEditor(contents.toUtf8());
    QVERIFY(otherWidget);
    Core::IEditor *otherEditor = m_editor;
    const QScopeGuard closeOther([otherEditor] {
        Core::EditorManager::closeEditors({otherEditor}, false);
    });
    TextEditorWidget *editorWidget = openEditor("current");
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });
    QCOMPARE(TextEditorWidget::currentTextEditorWidget(), editorWidget);

    // Like a refactoring that changes another open file.
    QTextCursor edit(otherWidget->document());
    edit.setPosition(11);
    edit.insertText("x");
    QCOMPARE(otherWidget->textDocument()->plainText(), QString("first line\nxsecond line"));

    otherWidget->undo();

    QCOMPARE(otherWidget->textDocument()->plainText(), contents);
    const QTextCursor cursor = otherWidget->textCursor();
    QVERIFY(!cursor.hasSelection());
    QCOMPARE(cursor.position(), 11);
}

class NonTextDocument final : public Core::IDocument
{
public:
    NonTextDocument() { setId("TextEditor.Test.NonTextEditor"); }

    Utils::Result<> setContents(const QByteArray &) final { return Utils::ResultOk; }
};

// Like the Android manifest editor, which embeds a text editor in a widget of its own.
class NonTextEditor final : public Core::IEditor
{
public:
    NonTextEditor()
    {
        setWidget(m_widget.get());
        m_embeddedWidget->setTextDocument(TextDocumentPtr(new TextDocument));
    }

    Core::IDocument *document() const final { return m_document.get(); }
    QWidget *toolBar() final { return nullptr; }
    TextEditorWidget *embeddedWidget() const { return m_embeddedWidget; }

private:
    const std::unique_ptr<NonTextDocument> m_document = std::make_unique<NonTextDocument>();
    const std::unique_ptr<QWidget> m_widget = std::make_unique<QWidget>();
    TextEditorWidget *const m_embeddedWidget = new TextEditorWidget(m_widget.get());
};

class NonTextEditorFactory final : public Core::IEditorFactory
{
public:
    NonTextEditorFactory()
    {
        setId("TextEditor.Test.NonTextEditor");
        setDisplayName("Non-Text Editor");
        setEditorCreator([] { return new NonTextEditor; });
    }
};

void UndoCursorTest::testUndoOfEditWhileNonTextEditorIsCurrent()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    const NonTextEditorFactory factory;
    QString title = "non_text";
    Core::IEditor *nonTextEditor
        = Core::EditorManager::openEditorWithContents(factory.id(), &title, {});
    QVERIFY(nonTextEditor);
    const QScopeGuard closeNonText([nonTextEditor] {
        Core::EditorManager::closeEditors({nonTextEditor}, false);
    });
    QCOMPARE(Core::EditorManager::currentEditor(), nonTextEditor);
    QCOMPARE(TextEditorWidget::currentTextEditorWidget(), nullptr);

    // Like a Replace All in open files while a form or diff editor is current.
    QTextCursor edit(editorWidget->document());
    edit.setPosition(11);
    edit.insertText("x");
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("first line\nxsecond line"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    const QTextCursor cursor = editorWidget->textCursor();
    QVERIFY(!cursor.hasSelection());
    QCOMPARE(cursor.position(), 11);
}

void UndoCursorTest::testUndoInEmbeddedWidget()
{
    const NonTextEditorFactory factory;
    QString title = "non_text";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(factory.id(), &title, {});
    QVERIFY(editor);
    const QScopeGuard cleanup([editor] { Core::EditorManager::closeEditors({editor}, false); });
    QCOMPARE(Core::EditorManager::currentEditor(), editor);

    TextEditorWidget *editorWidget = static_cast<NonTextEditor *>(editor)->embeddedWidget();
    const QString contents = "first line\nsecond line";
    QVERIFY(editorWidget->textDocument()->setPlainText(contents));
    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(0);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);
    QVERIFY(!editorWidget->hasFocus());

    QTest::keyClick(editorWidget, Qt::Key_X);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("x line\nsecond line"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    cursor = editorWidget->textCursor();
    QCOMPARE(cursor.anchor(), 0);
    QCOMPARE(cursor.position(), 5);
}

void UndoCursorTest::testUndoInFocusedWidget()
{
    TextEditorWidget *currentWidget = openEditor("current");
    QVERIFY(currentWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    // Like the text editor of a settings page or of another mode.
    TextEditorWidget editorWidget;
    editorWidget.setTextDocument(TextDocumentPtr(new TextDocument));
    const QString contents = "first line\nsecond line";
    QVERIFY(editorWidget.textDocument()->setPlainText(contents));
    editorWidget.show();
    editorWidget.activateWindow();
    if (!QTest::qWaitForWindowActive(&editorWidget))
        QSKIP("The window of the text editor does not become active.");
    editorWidget.setFocus();
    QVERIFY(editorWidget.hasFocus());
    QCOMPARE(TextEditorWidget::currentTextEditorWidget(), currentWidget);

    QTextCursor cursor = editorWidget.textCursor();
    cursor.setPosition(0);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget.setTextCursor(cursor);
    QTest::keyClick(&editorWidget, Qt::Key_X);
    QCOMPARE(editorWidget.textDocument()->plainText(), QString("x line\nsecond line"));

    editorWidget.undo();

    QCOMPARE(editorWidget.textDocument()->plainText(), contents);
    cursor = editorWidget.textCursor();
    QCOMPARE(cursor.anchor(), 0);
    QCOMPARE(cursor.position(), 5);
}

QObject *createUndoCursorTest()
{
    return new UndoCursorTest;
}

// Regression test for QTCREATORBUG-31149: reflowing a comment must not pull
// adjacent code lines into the paragraph.
class RewrapParagraphTest final : public QObject
{
    Q_OBJECT

private slots:
    void testRewrapParagraph_data();
    void testRewrapParagraph();
    void testRewrapSelection_data();
    void testRewrapSelection();
};

void RewrapParagraphTest::testRewrapParagraph_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("cursorBlock");
    QTest::addColumn<QString>("expected");

    // A short single-line comment surrounded by code stays untouched; the
    // code lines must not be merged into it.
    QTest::newRow("singleLineCommentAmongCode")
        << "int a = 1;\n// hello world\nint b = 2;" << 1
        << "int a = 1;\n// hello world\nint b = 2;";

    // Consecutive "//" comment lines reflow together, but the surrounding
    // code is left alone.
    QTest::newRow("multiLineCommentAmongCode")
        << "int a = 1;\n// alpha beta\n// gamma delta\nint b = 2;" << 1
        << "int a = 1;\n// alpha beta gamma delta\nint b = 2;";

    // Plain-text paragraphs are still bounded by blank lines only.
    QTest::newRow("plainTextParagraph")
        << "one two\nthree four\n\nnext" << 0
        << "one two three four\n\nnext";

    // Doxygen "///" comments keep their leader when reflowed.
    QTest::newRow("doxygenComment")
        << "/// alpha beta\n/// gamma delta\n" << 0
        << "/// alpha beta gamma delta\n";

    // A single "//" comment line too long for the margin wraps into several
    // lines that each keep the "//" leader (QTCREATORBUG-5674).
    QTest::newRow("singleLineCommentWraps")
        << "// aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa "
           "aaaaaaaaa aaaaaaaaa\n" << 0
        << "// aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa "
           "aaaaaaaaa\n// aaaaaaaaa\n";

    // In a "/** */" block the line following the paragraph is a bare "*" or
    // the closing "*/", neither of which shares the space that separates the
    // leader from the text. That space belongs to the leader nonetheless: a
    // paragraph that needs no wrapping comes back unchanged.
    QTest::newRow("doxygenBlockKeepsLeaderSpace")
        << "/**\n * @brief alpha beta\n */\n" << 1
        << "/**\n * @brief alpha beta\n */\n";

    // The separator is taken from the line as it is, so a tab survives as one.
    QTest::newRow("doxygenBlockKeepsLeaderTab")
        << "/**\n *\t@brief alpha beta\n */\n" << 1
        << "/**\n *\t@brief alpha beta\n */\n";

    // And one that does need wrapping keeps the leader on the new line.
    QTest::newRow("doxygenBlockWraps")
        << "/**\n  * @brief function is a function that calculates some result "
           "from the parameter d, but does not return the result\n  *\n"
           "  * @param d is the parameter that is used in calculation.\n  */\n" << 1
        << "/**\n  * @brief function is a function that calculates some result "
           "from the\n  * parameter d, but does not return the result\n  *\n"
           "  * @param d is the parameter that is used in calculation.\n  */\n";

    // The recovered space is written out, but not required of the other lines:
    // a continuation line without it must not leave its leader behind as text.
    QTest::newRow("doxygenBlockMixedLeaderSpacing")
        << "  * alpha beta\n  *gamma delta\n" << 0
        << "  * alpha beta gamma delta\n";
}

void RewrapParagraphTest::testRewrapParagraph()
{
    QFETCH(QString, input);
    QFETCH(int, cursorBlock);
    QFETCH(QString, expected);

    QString title = "rewrap.txt";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, input.toUtf8());
    QVERIFY(editor);
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *editorWidget = baseEditor->editorWidget();
    QVERIFY(editorWidget);

    MarginSettingsData margin = editorWidget->marginSettings();
    margin.m_marginColumn = 80;
    editorWidget->setMarginSettings(margin);

    QTextCursor cursor = editorWidget->textCursor();
    cursor.movePosition(QTextCursor::Start);
    for (int i = 0; i < cursorBlock; ++i)
        cursor.movePosition(QTextCursor::NextBlock);
    editorWidget->setTextCursor(cursor);

    editorWidget->rewrapParagraph();

    QCOMPARE(editorWidget->textDocument()->plainText(), expected);

    Core::EditorManager::closeEditors({editor}, false);
}

void RewrapParagraphTest::testRewrapSelection_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("anchorBlock");
    QTest::addColumn<int>("cursorBlock");
    QTest::addColumn<QString>("expected");

    // Selecting a whole comment reflows all of its paragraphs, whichever end
    // of the selection the cursor is at, and leaves the blank comment line
    // that separates them alone.
    QTest::newRow("wholeCommentWithCursorAtTheEnd")
        << "/**\n  * @brief function is a function that calculates some result "
           "from the parameter d, but does not return the result\n  *\n"
           "  * @param d is the parameter that is used in the calculation of "
           "the result that is returned\n  */\n" << 0 << 4
        << "/**\n  * @brief function is a function that calculates some result "
           "from the\n  * parameter d, but does not return the result\n  *\n"
           "  * @param d is the parameter that is used in the calculation of "
           "the result\n  * that is returned\n  */\n";

    // Plain-text paragraphs delimited by a blank line are reflowed one by one,
    // and each of them as a whole even where the selection covers only part of
    // it: the paragraph, not the selection, is the unit.
    QTest::newRow("selectionOverSeveralPlainTextParagraphs")
        << "one two\nthree four\n\nfive six\nseven eight\n" << 1 << 4
        << "one two three four\n\nfive six seven eight\n";

    // Code and a run of "//" comment lines are never one paragraph, so a
    // selection covering both reflows the comment without absorbing the code.
    QTest::newRow("selectionOverCodeAndComment")
        << "int a = 1;\n// alpha beta\n// gamma delta\nint b = 2;\n" << 0 << 4
        << "int a = 1;\n// alpha beta gamma delta\nint b = 2;\n";

    // A run of plain statements carries no signal - no blank line, no comment
    // leader - that tells it apart from a run of prose lines, so a selection
    // over it reflows the same way a plain-text paragraph does: this is not a
    // new consequence of acting on a selection, it already happened with a
    // single cursor placed in the middle of such a run.
    QTest::newRow("selectionOverPlainCodeStatements")
        << "int a = 1;\nint b = 2;\n" << 0 << 2
        << "int a = 1; int b = 2;\n";
}

void RewrapParagraphTest::testRewrapSelection()
{
    QFETCH(QString, input);
    QFETCH(int, anchorBlock);
    QFETCH(int, cursorBlock);
    QFETCH(QString, expected);

    QString title = "rewrap.txt";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, input.toUtf8());
    QVERIFY(editor);
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *editorWidget = baseEditor->editorWidget();
    QVERIFY(editorWidget);

    MarginSettingsData margin = editorWidget->marginSettings();
    margin.m_marginColumn = 80;
    editorWidget->setMarginSettings(margin);

    QTextCursor cursor = editorWidget->textCursor();
    cursor.movePosition(QTextCursor::Start);
    for (int i = 0; i < anchorBlock; ++i)
        cursor.movePosition(QTextCursor::NextBlock);
    for (int i = anchorBlock; i < cursorBlock; ++i)
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    if (anchorBlock == cursorBlock)
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    QVERIFY(cursor.hasSelection());
    editorWidget->setTextCursor(cursor);

    editorWidget->rewrapParagraph();

    QCOMPARE(editorWidget->textDocument()->plainText(), expected);

    Core::EditorManager::closeEditors({editor}, false);
}

QObject *createRewrapParagraphTest()
{
    return new RewrapParagraphTest;
}

class ExternalToolReadOnlyTest final : public QObject
{
    Q_OBJECT

private slots:
    void testReadOnlyEditorNotModified()
    {
        QString title = "readonly.txt";
        Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
            Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, "b\na\n");
        QVERIFY(editor);
        const QScopeGuard cleanup([&] { Core::EditorManager::closeEditors({editor}, false); });
        auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
        QVERIFY(baseEditor);
        TextEditorWidget *editorWidget = baseEditor->editorWidget();
        QVERIFY(editorWidget);

        editorWidget->setReadOnly(true);
        Core::ExternalToolManager::emitReplaceSelectionRequested("sorted");
        QCOMPARE(baseEditor->textDocument()->plainText(), QString("b\na\n"));

        editorWidget->setReadOnly(false);
        Core::ExternalToolManager::emitReplaceSelectionRequested("sorted");
        QVERIFY(baseEditor->textDocument()->plainText().contains("sorted"));
    }
};

QObject *createExternalToolReadOnlyTest()
{
    return new ExternalToolReadOnlyTest;
}

class RevertToSavedTest final : public QObject
{
    Q_OBJECT

private slots:
    void testRevertToSaved();
};

void RevertToSavedTest::testRevertToSaved()
{
    Utils::TemporaryDirectory tempDir("qtc-revert-test");
    QVERIFY(tempDir.isValid());
    const Utils::FilePath file = tempDir.path().pathAppended("revert.txt");
    const QString original = "line one\nline two\nline three";
    QVERIFY(file.writeFileContents(original.toUtf8()).has_value());

    Core::IEditor *editor = Core::EditorManager::openEditor(file);
    QVERIFY(editor);
    auto document = qobject_cast<TextDocument *>(editor->document());
    QVERIFY(document);
    QVERIFY(!document->isModified()); // "Revert to Saved" is disabled while unmodified

    QTextCursor cursor(document->document());
    cursor.insertText("modified ");
    QVERIFY(document->isModified());

    // The non-interactive core of the "Revert to Saved" action.
    const Utils::Result<> result = document->reload(Core::IDocument::FlagReload,
                                                    Core::IDocument::TypeContents);
    QVERIFY(result.has_value());
    QVERIFY(!document->isModified());
    QCOMPARE(document->plainText(), original);

    Core::EditorManager::closeEditors({editor}, false);
}

QObject *createRevertToSavedTest()
{
    return new RevertToSavedTest;
}

class FindReplaceTest final : public QObject
{
    Q_OBJECT

private slots:
    void testReplaceAll();
    void testReplaceSelected();
};

static BaseTextEditor *openTextEditor(const QString &contents, QString title)
{
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, contents.toUtf8());
    return qobject_cast<BaseTextEditor *>(editor);
}

void FindReplaceTest::testReplaceAll()
{
    const QString content = "int window;\nWindow w;\nTriangleWindow tw;";
    BaseTextEditor *editor = openTextEditor(content, "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();

    // Case-insensitive by default, so every "window"/"Window" is replaced.
    Core::BaseTextFind find(editorWidget);
    const int count = find.replaceAll("window", "find", {});

    QCOMPARE(count, 3);
    QCOMPARE(editorWidget->textDocument()->plainText(),
             QString("int find;\nfind w;\nTrianglefind tw;"));

    Core::EditorManager::closeEditors({editor}, false);
}

void FindReplaceTest::testReplaceSelected()
{
    const QString content = "int window;\nWindow w;\nTriangleWindow tw;";
    BaseTextEditor *editor = openTextEditor(content, "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();

    // Select the "Window" on the second line and replace only that occurrence.
    QTextCursor cursor(editorWidget->document());
    cursor.setPosition(12);
    cursor.setPosition(18, QTextCursor::KeepAnchor);
    QCOMPARE(cursor.selectedText(), QString("Window"));
    editorWidget->setTextCursor(cursor);

    Core::BaseTextFind find(editorWidget);
    find.replace("Window", "find", {});

    QCOMPARE(editorWidget->textDocument()->plainText(),
             QString("int window;\nfind w;\nTriangleWindow tw;"));

    Core::EditorManager::closeEditors({editor}, false);
}

QObject *createFindReplaceTest()
{
    return new FindReplaceTest;
}

class SnippetTest final : public QObject
{
    Q_OBJECT

private slots:
    void testVariableMirroring();
};

void SnippetTest::testVariableMirroring()
{
    BaseTextEditor *editor = openTextEditor(QString(), "snippet.cpp");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();

    // A class template snippet echoing the "class derived from QObject" snippet:
    // the "name" variable occurs three times and must stay in sync, and a
    // second "base" variable is reached by tabbing to the next placeholder.
    const QString snippet = "class $name$ : public $base$ {\n"
                            "    Q_OBJECT\n"
                            "public:\n"
                            "    $name$() {}\n"
                            "    virtual ~$name$() {}\n"
                            "};";
    editorWidget->insertCodeSnippet(0, snippet, &Snippet::parse);

    // The first variable is active; typing a value renames every occurrence.
    QTest::keyClicks(editorWidget, "MyName");

    // Tab jumps to the next placeholder ("base"); fill it too.
    QTest::keyClick(editorWidget, Qt::Key_Tab);
    QTest::keyClicks(editorWidget, "QObject");

    const QString text = editorWidget->textDocument()->plainText();
    QCOMPARE(text.count("MyName"), 3);
    QVERIFY(text.contains("public QObject"));
    QVERIFY(!text.contains("name")); // the "name" placeholder was filled everywhere
    QVERIFY(!text.contains("base")); // the "base" placeholder was reached via Tab

    Core::EditorManager::closeEditors({editor}, false);
}

QObject *createSnippetTest()
{
    return new SnippetTest;
}

class PrintTest final : public QObject
{
    Q_OBJECT

private slots:
    void testPrintsSelectedLinesOnly()
    {
        const QString all = "line1\nline2\nline3\nline4\n";
        QString title = "print.txt";
        Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
            Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, all.toUtf8());
        QVERIFY(editor);
        const QScopeGuard cleanup([&] { Core::EditorManager::closeEditors({editor}, false); });
        auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
        QVERIFY(baseEditor);
        TextEditorWidget *editorWidget = baseEditor->editorWidget();
        QVERIFY(editorWidget);

        QCOMPARE(editorWidget->textToPrint(false), all);

        // A selection reaching into a line prints that line as a whole.
        QTextCursor cursor = editorWidget->textCursor();
        cursor.setPosition(all.indexOf("ne2"));
        cursor.setPosition(all.indexOf("ne3"), QTextCursor::KeepAnchor);
        editorWidget->setTextCursor(cursor);
        QCOMPARE(editorWidget->textToPrint(true), QString("line2\nline3"));

        // A selection ending where a line starts does not print that line.
        cursor.setPosition(all.indexOf("line2"));
        cursor.setPosition(all.indexOf("line3"), QTextCursor::KeepAnchor);
        editorWidget->setTextCursor(cursor);
        QCOMPARE(editorWidget->textToPrint(true), QString("line2"));

        // Without a selection the whole document is printed.
        cursor.clearSelection();
        editorWidget->setTextCursor(cursor);
        QCOMPARE(editorWidget->textToPrint(false), all);
    }
};

QObject *createPrintTest()
{
    return new PrintTest;
}

class FollowSymbolTest final : public QObject
{
    Q_OBJECT

private slots:
    void testAnswerWithoutLookup()
    {
        QString title = "follow_symbol.txt";
        Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
            Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, "nothing to follow\n");
        QVERIFY(editor);
        const QScopeGuard cleanup([&] { Core::EditorManager::closeEditors({editor}, false); });
        auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
        QVERIFY(baseEditor);
        TextEditorWidget *editorWidget = baseEditor->editorWidget();
        QVERIFY(editorWidget);

        std::optional<bool> opened;
        editorWidget->openLinkUnderCursor([&opened](bool wasOpened) { opened = wasOpened; });

        QTRY_VERIFY(opened.has_value());
        QVERIFY(!*opened);
    }
};

QObject *createFollowSymbolTest()
{
    return new FollowSymbolTest;
}

} // TextEditor::Internal

#include "texteditor_test.moc"

#endif // WITH_TESTS
