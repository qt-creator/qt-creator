// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/store.h>

QT_BEGIN_NAMESPACE
class QTextDocument;
class QTextCursor;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT TypingSettings
{
public:
    // This enum must match the indexes of tabKeyBehavior widget
    enum TabKeyBehavior {
        TabNeverIndents = 0,
        TabAlwaysIndents = 1,
        TabLeadingWhitespaceIndents = 2
    };

    // This enum must match the indexes of smartBackspaceBehavior widget
    enum SmartBackspaceBehavior {
        BackspaceNeverIndents = 0,
        BackspaceFollowsPreviousIndents = 1,
        BackspaceUnindents = 2
    };

    enum CommentPosition {
        Automatic = 0,
        StartOfLine = 1,
        AfterWhitespace = 2,
    };

    TypingSettings();

    bool tabShouldIndent(const QTextDocument *document, const QTextCursor &cursor, int *suggestedPosition) const;

    Utils::Store toMap() const;
    void fromMap(const Utils::Store &map);

    bool equals(const TypingSettings &ts) const;

    friend bool operator==(const TypingSettings &t1, const TypingSettings &t2) { return t1.equals(t2); }
    friend bool operator!=(const TypingSettings &t1, const TypingSettings &t2) { return !t1.equals(t2); }

    bool m_autoIndent;
    TabKeyBehavior m_tabKeyBehavior;
    SmartBackspaceBehavior m_smartBackspaceBehavior;

    bool m_preferSingleLineComments;
    CommentPosition m_commentPosition = Automatic;
};

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::TypingSettings)
