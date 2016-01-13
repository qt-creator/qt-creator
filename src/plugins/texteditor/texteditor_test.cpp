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

#ifdef WITH_TESTS

#include <QApplication>
#include <QClipboard>
#include <QString>
#include <QtTest/QtTest>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>

#include "texteditor.h"
#include "texteditorplugin.h"
#include "textdocument.h"
#include "tabsettings.h"

using namespace TextEditor;

enum TransFormationType { Uppercase, Lowercase };

struct TestBlockSelection
{
    int positionBlock;
    int positionColumn;
    int anchorBlock;
    int anchorColumn;
    TestBlockSelection(int positionBlock, int positionColumn, int anchorBlock, int anchorColumn)
        : positionBlock(positionBlock), positionColumn(positionColumn)
        , anchorBlock(anchorBlock), anchorColumn(anchorColumn) {}
    TestBlockSelection() {}
};

Q_DECLARE_METATYPE(TransFormationType)
Q_DECLARE_METATYPE(TestBlockSelection)

void Internal::TextEditorPlugin::testBlockSelectionTransformation_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("transformedText");
    QTest::addColumn<TestBlockSelection>("selection");
    QTest::addColumn<TransFormationType>("type");

    QTest::newRow("uppercase")
            << QString::fromLatin1("aaa\nbbb\nccc\n")
            << QString::fromLatin1("aAa\nbBb\ncCc\n")
            << TestBlockSelection(0, 1, 2, 2)
            << Uppercase;
    QTest::newRow("lowercase")
            << QString::fromLatin1("AAA\nBBB\nCCC\n")
            << QString::fromLatin1("AaA\nBbB\nCcC\n")
            << TestBlockSelection(0, 1, 2, 2)
            << Lowercase;
    QTest::newRow("uppercase_leading_tabs")
            << QString::fromLatin1("\taaa\n" "\tbbb\n" "\tccc\n")
            << QString::fromLatin1("\taAa\n" "\tbBb\n" "\tcCc\n")
            << TestBlockSelection(0, 9, 2, 10)
            << Uppercase;
    QTest::newRow("lowercase_leading_tabs")
            << QString::fromLatin1("\tAAA\n\tBBB\n\tCCC\n")
            << QString::fromLatin1("\tAaA\n\tBbB\n\tCcC\n")
            << TestBlockSelection(0, 9, 2, 10)
            << Lowercase;
    QTest::newRow("uppercase_mixed_leading_whitespace")
            << QString::fromLatin1("\taaa\n    bbbbb\n    ccccc\n")
            << QString::fromLatin1("\tAaa\n    bbbbB\n    ccccC\n")
            << TestBlockSelection(0, 8, 2, 9)
            << Uppercase;
    QTest::newRow("lowercase_mixed_leading_whitespace")
            << QString::fromLatin1("\tAAA\n    BBBBB\n    CCCCC\n")
            << QString::fromLatin1("\taAA\n    BBBBb\n    CCCCc\n")
            << TestBlockSelection(0, 8, 2, 9)
            << Lowercase;
    QTest::newRow("uppercase_mid_tabs1")
            << QString::fromLatin1("a\ta\nbbbbbbbbb\nccccccccc\n")
            << QString::fromLatin1("a\ta\nbBBBBBbbb\ncCCCCCccc\n")
            << TestBlockSelection(0, 1, 2, 6)
            << Uppercase;
    QTest::newRow("lowercase_mid_tabs2")
            << QString::fromLatin1("AA\taa\n\t\t\nccccCCCCCC\n")
            << QString::fromLatin1("Aa\taa\n\t\t\ncccccccccC\n")
            << TestBlockSelection(0, 1, 2, 9)
            << Lowercase;
    QTest::newRow("uppercase_over_line_ending")
            << QString::fromLatin1("aaaaa\nbbb\nccccc\n")
            << QString::fromLatin1("aAAAa\nbBB\ncCCCc\n")
            << TestBlockSelection(0, 1, 2, 4)
            << Uppercase;
    QTest::newRow("lowercase_over_line_ending")
            << QString::fromLatin1("AAAAA\nBBB\nCCCCC\n")
            << QString::fromLatin1("AaaaA\nBbb\nCcccC\n")
            << TestBlockSelection(0, 1, 2, 4)
            << Lowercase;
}

void Internal::TextEditorPlugin::testBlockSelectionTransformation()
{
    // fetch test data
    QFETCH(QString, input);
    QFETCH(QString, transformedText);
    QFETCH(TestBlockSelection, selection);
    QFETCH(TransFormationType, type);

    // open editor
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
                Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, 0, input.toLatin1());
    QVERIFY(editor);
    if (BaseTextEditor *textEditor = qobject_cast<BaseTextEditor*>(editor)) {
        TextEditorWidget *editorWidget = textEditor->editorWidget();
        editorWidget->setBlockSelection(selection.positionBlock,
                                        selection.positionColumn,
                                        selection.anchorBlock,
                                        selection.anchorColumn);
        editorWidget->update();

        // transform blockselection
        switch (type) {
        case Uppercase:
            editorWidget->uppercaseSelection();
            break;
        case Lowercase:
            editorWidget->lowercaseSelection();
            break;
        }
        QCOMPARE(textEditor->textDocument()->plainText(), transformedText);
    }
    Core::EditorManager::closeDocument(editor->document(), false);
}

static const char text[] =
        "first  line\n"
        "second line\n"
        "third  line\n"
        "\n"
        "longest line in this text\n"
        "    leading space\n"
        ""    "\tleading tab\n"
        "mid\t" "tab\n"
        "endtab\t\n";

void Internal::TextEditorPlugin::testBlockSelectionInsert_data()
{
    QTest::addColumn<QString>("transformedText");
    QTest::addColumn<TestBlockSelection>("selection");
    QTest::addColumn<QString>("insertText");

    QTest::newRow("insert")
            << QString::fromLatin1(".first  line\n"
                                   ".second line\n"
                                   ".third  line\n"
                                   "\n"
                                   "longest line in this text\n"
                                   "    leading space\n"
                                   ""    "\tleading tab\n"
                                   "mid\t" "tab\n"
                                   "endtab\t\n")
            << TestBlockSelection(0, 0, 2, 0)
            << QString::fromLatin1(".");
    QTest::newRow("insert after line end 1")
            << QString::fromLatin1("first  line\n"
                                   "second line\n"
                                   "third.  line\n"
                                   "     .\n"
                                   "longe.st line in this text\n"
                                   "    leading space\n"
                                   ""    "\tleading tab\n"
                                   "mid\t" "tab\n"
                                   "endtab\t\n")
            << TestBlockSelection(2, 5, 4, 5)
            << QString::fromLatin1(".");
    QTest::newRow("insert after line end 2")
            << QString::fromLatin1("first  line                     .\n"
                                   "second line                     .\n"
                                   "third  line                     .\n"
                                   "                                .\n"
                                   "longest line in this text       .\n"
                                   "    leading space               .\n"
                                   ""    "\tleading tab             .\n"
                                   "mid\t" "tab                     .\n"
                                   "endtab\t                        .\n")
            << TestBlockSelection(0, 32, 8, 32)
            << QString::fromLatin1(".");
    QTest::newRow("insert in leading tab")
            << QString::fromLatin1("first  line\n"
                                   "second line\n"
                                   "third  line\n"
                                   "\n"
                                   "lo.ngest line in this text\n"
                                   "  .  leading space\n"
                                   "  .      leading tab\n"
                                   "mid\t" "tab\n"
                                   "endtab\t\n")
            << TestBlockSelection(4, 2, 6, 2)
            << QString::fromLatin1(".");
    QTest::newRow("insert in middle tab")
            << QString::fromLatin1("first  line\n"
                                   "second line\n"
                                   "third  line\n"
                                   "\n"
                                   "longest line in this text\n"
                                   "    leading space\n"
                                   "     .   leading tab\n"
                                   "mid  .   tab\n"
                                   "endta.b\t\n")

            << TestBlockSelection(6, 5, 8, 5)
            << QString::fromLatin1(".");
    QTest::newRow("insert same block count with all blocks same length")
            << QString::fromLatin1(".first  line\n"
                                   ".second line\n"
                                   ".third  line\n"
                                   "\n"
                                   "longest line in this text\n"
                                   "    leading space\n"
                                   ""    "\tleading tab\n"
                                   "mid\t" "tab\n"
                                   "endtab\t\n")
            << TestBlockSelection(0, 0, 2, 0)
            << QString::fromLatin1(".\n.\n.");
    QTest::newRow("insert same block count with all blocks different length")
            << QString::fromLatin1(".  first  line\n"
                                   ".. second line\n"
                                   "...third  line\n"
                                   "\n"
                                   "longest line in this text\n"
                                   "    leading space\n"
                                   ""    "\tleading tab\n"
                                   "mid\t" "tab\n"
                                   "endtab\t\n")
            << TestBlockSelection(0, 0, 2, 0)
            << QString::fromLatin1(".\n..\n...");
    QTest::newRow("insert same block count with some blocks containing tabs")
            << QString::fromLatin1(".        first  line\n"
                                   ".\t second line\n"
                                   ".\t.third  line\n"
                                   "\n"
                                   "longest line in this text\n"
                                   "    leading space\n"
                                   ""    "\tleading tab\n"
                                   "mid\t" "tab\n"
                                   "endtab\t\n")
            << TestBlockSelection(0, 0, 2, 0)
            << QString::fromLatin1(".\n.\t\n.\t.");
    QTest::newRow("insert same block count with some blocks containing tabs in mid line")
            << QString::fromLatin1("fi.      rst  line\n"
                                   "se.\t cond line\n"
                                   "th.\t.ird  line\n"
                                   "\n"
                                   "longest line in this text\n"
                                   "    leading space\n"
                                   ""    "\tleading tab\n"
                                   "mid\t" "tab\n"
                                   "endtab\t\n")
            << TestBlockSelection(0, 2, 2, 2)
            << QString::fromLatin1(".\n.\t\n.\t.");
    QTest::newRow("insert different block count with all blocks same length")
            << QString::fromLatin1(".first  line\n"
                                   ".\n"
                                   ".second line\n"
                                   ".\n"
                                   ".third  line\n"
                                   ".\n"
                                   "\n"
                                   "longest line in this text\n"
                                   "    leading space\n"
                                   ""    "\tleading tab\n"
                                   "mid\t" "tab\n"
                                   "endtab\t\n")
            << TestBlockSelection(0, 0, 2, 0)
            << QString::fromLatin1(".\n.");
    QTest::newRow("insert different block count with all blocks different length")
            << QString::fromLatin1(". first  line\n"
                                   "..\n"
                                   ". second line\n"
                                   "..\n"
                                   ". third  line\n"
                                   "..\n"
                                   "\n"
                                   "longest line in this text\n"
                                   "    leading space\n"
                                   ""    "\tleading tab\n"
                                   "mid\t" "tab\n"
                                   "endtab\t\n")
            << TestBlockSelection(0, 0, 2, 0)
            << QString::fromLatin1(".\n..");
    QTest::newRow("insert different block count with some blocks containing tabs")
            << QString::fromLatin1(".        first  line\n"
                                   ".\t \n"
                                   ".\t.\n"
                                   ".        second line\n"
                                   ".\t \n"
                                   ".\t.\n"
                                   "third  line\n"
                                   "\n"
                                   "longest line in this text\n"
                                   "    leading space\n"
                                   ""    "\tleading tab\n"
                                   "mid\t" "tab\n"
                                   "endtab\t\n")
            << TestBlockSelection(0, 0, 1, 0)
            << QString::fromLatin1(".\n.\t\n.\t.");
    QTest::newRow("insert different block count with some blocks containing tabs in mid line")
            << QString::fromLatin1("fi.      rst  line\n"
                                   "  .\t \n"
                                   "  .\t.\n"
                                   "se.      cond line\n"
                                   "  .\t \n"
                                   "  .\t.\n"
                                   "third  line\n"
                                   "\n"
                                   "longest line in this text\n"
                                   "    leading space\n"
                                   ""    "\tleading tab\n"
                                   "mid\t" "tab\n"
                                   "endtab\t\n")
            << TestBlockSelection(0, 2, 1, 2)
            << QString::fromLatin1(".\n.\t\n.\t.");
}

void Internal::TextEditorPlugin::testBlockSelectionInsert()
{
    // fetch test data
    QFETCH(QString, transformedText);
    QFETCH(TestBlockSelection, selection);
    QFETCH(QString, insertText);

    // open editor
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
                Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, 0, text);
    QVERIFY(editor);
    if (BaseTextEditor *textEditor = qobject_cast<BaseTextEditor*>(editor)) {
        TextEditorWidget *editorWidget = textEditor->editorWidget();
        editorWidget->setBlockSelection(selection.positionBlock,
                                        selection.positionColumn,
                                        selection.anchorBlock,
                                        selection.anchorColumn);
        editorWidget->update();
        editorWidget->insertPlainText(insertText);

        QCOMPARE(textEditor->textDocument()->plainText(), transformedText);
    }
    Core::EditorManager::closeDocument(editor->document(), false);
}


void Internal::TextEditorPlugin::testBlockSelectionRemove_data()
{
    QTest::addColumn<QString>("transformedText");
    QTest::addColumn<TestBlockSelection>("selection");
    QTest::newRow("Delete")
            << QString::fromLatin1("first  ine\n"
                                   "second ine\n"
                                   "third  ine\n"
                                   "\n"
                                   "longest line in this text\n"
                                   "    leading space\n"
                                   ""    "\tleading tab\n"
                                   "mid\t" "tab\n"
                                   "endtab\t\n")
            << TestBlockSelection(0, 7, 2, 8);
    QTest::newRow("Delete All")
            << QString::fromLatin1("\n\n\n\n\n\n\n\n\n")
            << TestBlockSelection(0, 0, 8, 30);
    QTest::newRow("Delete Inside Tab")
            << QString::fromLatin1("first  line\n"
                                   "second line\n"
                                   "third  line\n"
                                   "\n"
                                   "longest line in this text\n"
                                   "   leading space\n"
                                   "       leading tab\n"
                                   "mi\t" "tab\n"
                                   "endtab\t\n")
            << TestBlockSelection(5, 2, 7, 3);
    QTest::newRow("Delete around Tab")
            << QString::fromLatin1("first  line\n"
                                   "second line\n"
                                   "third  line\n"
                                   "\n"
                                   "longest line in this text\n"
                                   "  ng space\n"
                                   "  eading tab\n"
                                   "miab\n"
                                   "endtab\t\n")
            << TestBlockSelection(5, 2, 7, 9);
    QTest::newRow("Delete behind text")
            << QString::fromLatin1("first  line\n"
                                   "second line\n"
                                   "third  line\n"
                                   "\n"
                                   "longest line in this text\n"
                                   "    leading space\n"
                                   ""    "\tleading tab\n"
                                   "mid\t" "tab\n"
                                   "endtab\t\n")
            << TestBlockSelection(0, 30, 8, 35);
}

void Internal::TextEditorPlugin::testBlockSelectionRemove()
{
    // fetch test data
    QFETCH(QString, transformedText);
    QFETCH(TestBlockSelection, selection);

    // open editor
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
                Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, 0, text);
    QVERIFY(editor);
    if (BaseTextEditor *textEditor = qobject_cast<BaseTextEditor*>(editor)) {
        TextEditorWidget *editorWidget = textEditor->editorWidget();
        editorWidget->setBlockSelection(selection.positionBlock,
                                        selection.positionColumn,
                                        selection.anchorBlock,
                                        selection.anchorColumn);
        editorWidget->update();
        editorWidget->insertPlainText(QString());

        QCOMPARE(textEditor->textDocument()->plainText(), transformedText);
    }
    Core::EditorManager::closeDocument(editor->document(), false);
}

void Internal::TextEditorPlugin::testBlockSelectionCopy_data()
{
    QTest::addColumn<QString>("copiedText");
    QTest::addColumn<TestBlockSelection>("selection");
    QTest::newRow("copy")
            << QString::fromLatin1("lin\n"
                                   "lin\n"
                                   "lin")
            << TestBlockSelection(0, 7, 2, 10);
    QTest::newRow("copy over line end")
            << QString::fromLatin1("ond line    \n"
                                   "rd  line    \n"
                                   "            ")
            << TestBlockSelection(1, 3, 3, 15);
    QTest::newRow("copy inside tab")
            << QString::fromLatin1("ond line    \n"
                                   "rd  line    \n"
                                   "            ")
            << TestBlockSelection(1, 3, 3, 15);
    QTest::newRow("copy start in tab")
            << QString::fromLatin1("gest lin\n"
                                   " leading\n"
                                   "     lea")
            << TestBlockSelection(4, 3, 6, 11);
    QTest::newRow("copy around tab")
            << QString::fromLatin1("  leadin\n"
                                   "      le\n"
                                   "d\tta")
            << TestBlockSelection(5, 2, 7, 10);
}

void Internal::TextEditorPlugin::testBlockSelectionCopy()
{
    // fetch test data
    QFETCH(QString, copiedText);
    QFETCH(TestBlockSelection, selection);

    // open editor
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
                Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, 0, text);
    QVERIFY(editor);
    if (BaseTextEditor *textEditor = qobject_cast<BaseTextEditor*>(editor)) {
        TextEditorWidget *editorWidget = textEditor->editorWidget();
        editorWidget->setBlockSelection(selection.positionBlock,
                                        selection.positionColumn,
                                        selection.anchorBlock,
                                        selection.anchorColumn);
        editorWidget->update();
        editorWidget->copy();

        QCOMPARE(qApp->clipboard()->text(), copiedText);
    }
    Core::EditorManager::closeDocument(editor->document(), false);
}

QString tabPolicyToString(TabSettings::TabPolicy policy)
{
    switch (policy) {
    case TabSettings::SpacesOnlyTabPolicy:
        return QLatin1String("spacesOnlyPolicy");
    case TabSettings::TabsOnlyTabPolicy:
        return QLatin1String("tabsOnlyPolicy");
    case TabSettings::MixedTabPolicy:
        return QLatin1String("mixedIndentPolicy");
    }
    return QString();
}

QString continuationAlignBehaviorToString(TabSettings::ContinuationAlignBehavior behavior)
{
    switch (behavior) {
    case TabSettings::NoContinuationAlign:
        return QLatin1String("noContinuation");
    case TabSettings::ContinuationAlignWithSpaces:
        return QLatin1String("spacesContinuation");
    case TabSettings::ContinuationAlignWithIndent:
        return QLatin1String("indentContinuation");
    }
    return QString();
}

struct TabSettingsFlags{
    TabSettings::TabPolicy policy;
    TabSettings::ContinuationAlignBehavior behavior;
};

typedef std::function<bool(TabSettingsFlags)> IsClean;
void generateTestRows(QLatin1String name, QString text, IsClean isClean)
{
    QList<TabSettings::TabPolicy> allPolicys;
    allPolicys << TabSettings::SpacesOnlyTabPolicy
               << TabSettings::TabsOnlyTabPolicy
               << TabSettings::MixedTabPolicy;
    QList<TabSettings::ContinuationAlignBehavior> allbehavior;
    allbehavior << TabSettings::NoContinuationAlign
                << TabSettings::ContinuationAlignWithSpaces
                << TabSettings::ContinuationAlignWithIndent;

    const QLatin1Char splitter('_');
    const int indentSize = 3;

    foreach (TabSettings::TabPolicy policy, allPolicys) {
        foreach (TabSettings::ContinuationAlignBehavior behavior, allbehavior) {
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

void Internal::TextEditorPlugin::testIndentationClean_data()
{
    QTest::addColumn<TabSettings::TabPolicy>("policy");
    QTest::addColumn<TabSettings::ContinuationAlignBehavior>("behavior");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("indentSize");
    QTest::addColumn<bool>("clean");

    generateTestRows(QLatin1String("emptyString"), QString::fromLatin1(""),
                     [](TabSettingsFlags) -> bool {
        return true;
    });

    generateTestRows(QLatin1String("spaceIndentation"), QString::fromLatin1("   f"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy != TabSettings::TabsOnlyTabPolicy;
    });

    generateTestRows(QLatin1String("spaceIndentationGuessTabs"), QString::fromLatin1("   f\n\tf"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy == TabSettings::SpacesOnlyTabPolicy;
    });

    generateTestRows(QLatin1String("tabIndentation"), QString::fromLatin1("\tf"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy == TabSettings::TabsOnlyTabPolicy;
    });

    generateTestRows(QLatin1String("tabIndentationGuessTabs"), QString::fromLatin1("\tf\n\tf"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy != TabSettings::SpacesOnlyTabPolicy;
    });

    generateTestRows(QLatin1String("doubleSpaceIndentation"), QString::fromLatin1("      f"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy != TabSettings::TabsOnlyTabPolicy
                && flags.behavior != TabSettings::NoContinuationAlign;
    });

    generateTestRows(QLatin1String("doubleTabIndentation"), QString::fromLatin1("\t\tf"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy == TabSettings::TabsOnlyTabPolicy
                && flags.behavior == TabSettings::ContinuationAlignWithIndent;
    });

    generateTestRows(QLatin1String("tabSpaceIndentation"), QString::fromLatin1("\t   f"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy == TabSettings::TabsOnlyTabPolicy
                && flags.behavior == TabSettings::ContinuationAlignWithSpaces;
    });
}

void Internal::TextEditorPlugin::testIndentationClean()
{
    // fetch test data
    QFETCH(TabSettings::TabPolicy, policy);
    QFETCH(TabSettings::ContinuationAlignBehavior, behavior);
    QFETCH(QString, text);
    QFETCH(int, indentSize);
    QFETCH(bool, clean);

    const TabSettings settings(policy, indentSize, indentSize, behavior);
    const QTextDocument doc(text);
    const QTextBlock block = doc.firstBlock();

    QCOMPARE(settings.isIndentationClean(block, indentSize), clean);
}

#endif // ifdef WITH_TESTS
