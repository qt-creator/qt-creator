// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "typingsettings.h"

#include <coreplugin/icore.h>

#include <QTextCursor>
#include <QTextDocument>

static const char autoIndentKey[] = "AutoIndent";
static const char tabKeyBehaviorKey[] = "TabKeyBehavior";
static const char smartBackspaceBehaviorKey[] = "SmartBackspaceBehavior";
static const char preferSingleLineCommentsKey[] = "PreferSingleLineComments";
static const char preferAfterWhitespaceCommentsKey[] = "PreferAfterWhitespaceComments";

using namespace Utils;

namespace TextEditor {

TypingSettings::TypingSettings():
    m_autoIndent(true),
    m_tabKeyBehavior(TabNeverIndents),
    m_smartBackspaceBehavior(BackspaceNeverIndents),
    m_preferSingleLineComments(false)
{
}

Store TypingSettings::toMap() const
{
    return {
        {autoIndentKey, m_autoIndent},
        {tabKeyBehaviorKey, m_tabKeyBehavior},
        {smartBackspaceBehaviorKey, m_smartBackspaceBehavior},
        {preferSingleLineCommentsKey, m_preferSingleLineComments},
        {preferAfterWhitespaceCommentsKey, m_commentPosition}
    };
}

void TypingSettings::fromMap(const Store &map)
{
    m_autoIndent = map.value(autoIndentKey, m_autoIndent).toBool();
    m_tabKeyBehavior = (TabKeyBehavior) map.value(tabKeyBehaviorKey, m_tabKeyBehavior).toInt();
    m_smartBackspaceBehavior = (SmartBackspaceBehavior)map.value(
                smartBackspaceBehaviorKey, m_smartBackspaceBehavior).toInt();
    m_preferSingleLineComments =
        map.value(preferSingleLineCommentsKey, m_preferSingleLineComments).toBool();
    m_commentPosition = CommentPosition(
        std::clamp(map.value(preferAfterWhitespaceCommentsKey, m_commentPosition).toInt(),
                   int(Automatic),
                   int(AfterWhitespace)));
}

bool TypingSettings::equals(const TypingSettings &ts) const
{
    return m_autoIndent == ts.m_autoIndent
           && m_tabKeyBehavior == ts.m_tabKeyBehavior
           && m_smartBackspaceBehavior == ts.m_smartBackspaceBehavior
           && m_preferSingleLineComments == ts.m_preferSingleLineComments
           && m_commentPosition == ts.m_commentPosition;
}

bool TypingSettings::tabShouldIndent(const QTextDocument *document,
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


} // namespace TextEditor
