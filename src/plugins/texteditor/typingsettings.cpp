// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "typingsettings.h"

#include "texteditorsettings.h"

#include <coreplugin/icore.h>

#include <QTextCursor>
#include <QTextDocument>

using namespace Utils;

namespace TextEditor {

TypingSettingsData::TypingSettingsData():
    m_autoIndent(true),
    m_tabKeyBehavior(TabNeverIndents),
    m_smartBackspaceBehavior(BackspaceUnindents),
    m_preferSingleLineComments(false)
{
}

TypingSettings::TypingSettings()
{
    setAutoApply(false);
    setSettingsGroup("textTypingSettings");

    autoIndent.setSettingsKey("AutoIndent");
    autoIndent.setDefaultValue(true);

    tabKeyBehavior.setSettingsKey("TabKeyBehavior");
    tabKeyBehavior.setDefaultValue(TypingSettingsData::TabNeverIndents);

    smartBackspaceBehavior.setDefaultValue(TypingSettingsData::BackspaceUnindents);
    smartBackspaceBehavior.setSettingsKey("SmartBackspaceBehavior");

    preferSingleLineComments.setSettingsKey("PreferSingleLineComments");
    preferSingleLineComments.setDefaultValue(false);

    commentPosition.setSettingsKey("PreferAfterWhitespaceComments");
}

void TypingSettings::setData(const TypingSettingsData &data)
{
    autoIndent.setValue(data.m_autoIndent);
    tabKeyBehavior.setValue(data.m_tabKeyBehavior);
    smartBackspaceBehavior.setValue(data.m_smartBackspaceBehavior);
    preferSingleLineComments.setValue(data.m_preferSingleLineComments);
    commentPosition.setValue(data.m_commentPosition);
}

TypingSettingsData TypingSettings::data() const
{
    TypingSettingsData d;
    d.m_autoIndent = autoIndent();
    d.m_tabKeyBehavior = tabKeyBehavior();
    d.m_smartBackspaceBehavior = smartBackspaceBehavior();
    d.m_preferSingleLineComments = preferSingleLineComments();
    d.m_commentPosition = TypingSettingsData::CommentPosition(
        std::clamp(int(commentPosition()),
                   int(TypingSettingsData::Automatic),
                   int(TypingSettingsData::AfterWhitespace)));
    return d;
}

bool TypingSettingsData::equals(const TypingSettingsData &ts) const
{
    return m_autoIndent == ts.m_autoIndent
           && m_tabKeyBehavior == ts.m_tabKeyBehavior
           && m_smartBackspaceBehavior == ts.m_smartBackspaceBehavior
           && m_preferSingleLineComments == ts.m_preferSingleLineComments
           && m_commentPosition == ts.m_commentPosition;
}

bool TypingSettingsData::tabShouldIndent(const QTextDocument *document,
                                     const QTextCursor &cursor,
                                     int *suggestedPosition) const
{
    if (m_tabKeyBehavior == TabNeverIndents)
        return false;
    QTextCursor tc = cursor;
    if (suggestedPosition)
        *suggestedPosition = tc.position(); // At least suggest original position
    tc.movePosition(QTextCursor::StartOfLine);
    if (tc.atBlockEnd()) // cursor was on a blank line
        return true;
    if (document->characterAt(tc.position()).isSpace()) {
        tc.movePosition(QTextCursor::WordRight);
        if (tc.positionInBlock() >= cursor.positionInBlock()) {
            if (suggestedPosition)
                *suggestedPosition = tc.position(); // Suggest position after whitespace
            if (m_tabKeyBehavior == TabLeadingWhitespaceIndents)
                return true;
        }
    }
    return (m_tabKeyBehavior == TabAlwaysIndents);
}

TypingSettings &globalTypingSettings()
{
    static TypingSettings theGlobalTypingSettings;
    return theGlobalTypingSettings;
}

void updateGlobalTypingSettings(const TypingSettingsData &newTypingSettings)
{
    if (newTypingSettings.equals(globalTypingSettings().data()))
        return;

    globalTypingSettings().setData(newTypingSettings);
    globalTypingSettings().writeSettings();

    emit TextEditorSettings::instance()->typingSettingsChanged(newTypingSettings);
}

void setupTypingSettings()
{
    globalTypingSettings().readSettings();
}

} // namespace TextEditor
