// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "storagesettings.h"
#include "tabsettings.h"
#include "textdocument.h"
#include "texteditor.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/filepath.h>
#include <utils/multitextcursor.h>

#include <QTest>
#include <QTextCursor>
#include <QTextDocument>

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
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<bool>("modified"); // whether cleaning is expected to change the document

    const QString tabBlankTab = "\t \t";  // trailing whitespace, as in the Squish test
    const QString tripleTab = "\t\t\t";   // whitespace-only line, as in the Squish test
    const QString defaultIgnore = "*.md, *.MD, Makefile";

    // A regular source file: trailing whitespace is stripped and the
    // whitespace-only line is emptied (the QTCREATORBUG-24565 scenario).
    QTest::newRow("regularFile")
        << "main.cpp" << defaultIgnore << true
        << ("foo" + tabBlankTab + "\n" + tripleTab + "\nbar\n")
        << QString("foo\n\nbar\n") << true;

    // A file matching the ignore list (*.md) keeps its trailing whitespace, but
    // its indentation-only line is still cleaned: the ignore list only gates
    // trailing-whitespace removal, not indentation cleaning. The Squish test only
    // ever checked the trailing-whitespace half of this.
    QTest::newRow("ignoredExtension")
        << "notes.md" << defaultIgnore << true
        << ("foo" + tabBlankTab + "\n" + tripleTab + "\nbar\n")
        << ("foo" + tabBlankTab + "\n\nbar\n") << true;

    // The ignore list also matches plain file names without an extension. With
    // nothing left to clean, the document stays unmodified (Squish checked this
    // via the editor's "*" modification marker).
    QTest::newRow("ignoredName")
        << "Makefile" << defaultIgnore << true
        << ("foo" + tabBlankTab + "\n")
        << ("foo" + tabBlankTab + "\n") << false;

    // With "skip trailing whitespace" disabled, trailing whitespace is trimmed
    // even for files on the ignore list (the counter-intuitive sense of the flag).
    QTest::newRow("alwaysTrimOverridesIgnoreList")
        << "notes.md" << defaultIgnore << false
        << ("foo" + tabBlankTab + "\n")
        << QString("foo\n") << true;
}

void CleanWhitespaceTest::testCleanWhitespace()
{
    QFETCH(QString, fileName);
    QFETCH(QString, ignoreFileTypes);
    QFETCH(bool, skipTrailingWhitespace);
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
    TabSettingsData tabSettings(TabSettingsData::TabsOnlyTabPolicy, 4, 4,
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

} // TextEditor::Internal

#include "texteditor_test.moc"

#endif // WITH_TESTS
