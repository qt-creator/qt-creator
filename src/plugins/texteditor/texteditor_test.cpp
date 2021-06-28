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

#include <QGuiApplication>
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

#endif // ifdef WITH_TESTS
