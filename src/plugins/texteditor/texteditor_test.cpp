// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "tabsettings.h"
#include "textdocument.h"

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

} // TextEditor::Internal

#include "texteditor_test.moc"

#endif // WITH_TESTS
