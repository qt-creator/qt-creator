// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/aspects.h>

QT_BEGIN_NAMESPACE
class QTextDocument;
class QTextCursor;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT TypingSettingsData
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

    bool tabShouldIndent(const QTextDocument *document, const QTextCursor &cursor, int *suggestedPosition) const;

    bool m_autoIndent = true;
    TabKeyBehavior m_tabKeyBehavior = TabNeverIndents;
    SmartBackspaceBehavior m_smartBackspaceBehavior = BackspaceUnindents;

    bool m_preferSingleLineComments = false;
    CommentPosition m_commentPosition = Automatic;
};

class TEXTEDITOR_EXPORT TypingSettings : public Utils::AspectContainer
{
public:
    TypingSettings();

    void apply() final;

    Utils::BoolAspect autoIndent{this};
    Utils::TypedSelectionAspect<TypingSettingsData::TabKeyBehavior> tabKeyBehavior{this};
    Utils::BoolAspect preferSingleLineComments{this};
    Utils::TypedSelectionAspect<TypingSettingsData::CommentPosition> commentPosition{this};
    Utils::TypedSelectionAspect<TypingSettingsData::SmartBackspaceBehavior> smartBackspaceBehavior{this};

    TypingSettingsData data() const;
    void setData(const TypingSettingsData &data);
};

void setupTypingSettings();

TEXTEDITOR_EXPORT TypingSettings &globalTypingSettings();

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::TypingSettingsData)
