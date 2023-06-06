// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "tabsettings.h"
#include "texteditorplugin.h"

#include <QTextDocument>
#include <QtTest/QtTest>

namespace TextEditor {

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

using IsClean = std::function<bool (TabSettingsFlags)>;
void generateTestRows(const QLatin1String &name, const QString &text, IsClean isClean)
{
    const QVector<TabSettings::TabPolicy> allPolicies = {
        TabSettings::SpacesOnlyTabPolicy,
        TabSettings::TabsOnlyTabPolicy,
        TabSettings::MixedTabPolicy
    };
    const QVector<TabSettings::ContinuationAlignBehavior> allbehaviors = {
        TabSettings::NoContinuationAlign,
        TabSettings::ContinuationAlignWithSpaces,
        TabSettings::ContinuationAlignWithIndent
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

} // namespace TextEditor

#endif // ifdef WITH_TESTS
