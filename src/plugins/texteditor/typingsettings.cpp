// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "typingsettings.h"

#include "texteditorsettings.h"
#include "texteditortr.h"

#include <QTextCursor>
#include <QTextDocument>

using namespace Utils;

namespace TextEditor {

TypingSettings::TypingSettings()
{
    setAutoApply(false);
    setSettingsGroup("textTypingSettings");

    autoIndent.setSettingsKey("AutoIndent");
    autoIndent.setDefaultValue(true);
    autoIndent.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    autoIndent.setLabelText(Tr::tr("Enable automatic &indentation"));

    tabKeyBehavior.setSettingsKey("TabKeyBehavior");
    tabKeyBehavior.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    tabKeyBehavior.addOption(Tr::tr("Never"));
    tabKeyBehavior.addOption(Tr::tr("Always"));
    tabKeyBehavior.addOption(Tr::tr("In Leading White Space"));
    tabKeyBehavior.setDefaultValue(TypingSettingsData::TabNeverIndents);
    tabKeyBehavior.setLabelText(Tr::tr("Tab key performs auto-indent:"));

    smartBackspaceBehavior.setSettingsKey("SmartBackspaceBehavior");
    smartBackspaceBehavior.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    smartBackspaceBehavior.addOption(Tr::tr("None", "Backspace indentation: None"));
    smartBackspaceBehavior.addOption(Tr::tr("Follows Previous Indents"));
    smartBackspaceBehavior.addOption(Tr::tr("Unindents"));
    smartBackspaceBehavior.setDefaultValue(TypingSettingsData::BackspaceUnindents);
    smartBackspaceBehavior.setLabelText(Tr::tr("Backspace indentation:"));
    smartBackspaceBehavior.setToolTip(Tr::tr("<html><head/><body>\n"
        "Specifies how backspace interacts with indentation.\n"
        "\n"
        "<ul>\n"
        "<li>None: No interaction at all. Regular plain backspace behavior.\n"
        "</li>\n"
        "\n"
        "<li>Follows Previous Indents: In leading white space it will take the cursor back to the nearest indentation level used in previous lines.\n"
        "</li>\n"
        "\n"
        "<li>Unindents: If the character behind the cursor is a space it behaves as a backtab.\n"
        "</li>\n"
        "</ul></body></html>\n"
        ));

    preferSingleLineComments.setSettingsKey("PreferSingleLineComments");
    preferSingleLineComments.setDefaultValue(false);
    preferSingleLineComments.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    preferSingleLineComments.setLabelText(Tr::tr("Prefer single line comments"));

    const QString automaticText = Tr::tr("Automatic");
    const QString lineStartText = Tr::tr("At Line Start");
    const QString afterWhitespaceText = Tr::tr("After Whitespace");

    const QString generalCommentPosition = Tr::tr(
        "Specifies where single line comments should be positioned.");
    const QString automaticCommentPosition
        = Tr::tr(
              "%1: The highlight definition for the file determines the position. If no highlight "
              "definition is available, the comment is placed after leading whitespaces.")
              .arg(automaticText);
    const QString lineStartCommentPosition
        = Tr::tr("%1: The comment is placed at the start of the line.").arg(lineStartText);
    const QString afterWhitespaceCommentPosition
        = Tr::tr("%1: The comment is placed after leading whitespaces.").arg(afterWhitespaceText);

    commentPosition.setSettingsKey("PreferAfterWhitespaceComments");
    commentPosition.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    commentPosition.addOption(automaticText);
    commentPosition.addOption(lineStartText);
    commentPosition.addOption(afterWhitespaceText);
    commentPosition.setLabelText(Tr::tr("Preferred comment position:"));
    commentPosition.setToolTip(QString("<html><head/><body>\n"
                                                   "%1\n"
                                                   "\n"
                                                   "<ul>\n"
                                                   "<li>%2\n"
                                                   "</li>\n"
                                                   "\n"
                                                   "<li>%3\n"
                                                   "</li>\n"
                                                   "\n"
                                                   "<li>%4\n"
                                                   "</li>\n"
                                                   "</ul></body></html>\n")
                                               .arg(generalCommentPosition)
                                               .arg(automaticCommentPosition)
                                               .arg(lineStartCommentPosition)
                                               .arg(afterWhitespaceCommentPosition));
}

void TypingSettings::setData(const TypingSettingsData &data)
{
    autoIndent.setValue(data.m_autoIndent);
    tabKeyBehavior.setValue(data.m_tabKeyBehavior);
    smartBackspaceBehavior.setValue(data.m_smartBackspaceBehavior);
    preferSingleLineComments.setValue(data.m_preferSingleLineComments);
    commentPosition.setValue(data.m_commentPosition);
}

void TypingSettings::apply()
{
    AspectContainer::apply();
    emit TextEditorSettings::instance()->typingSettingsChanged(data());
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

void setupTypingSettings()
{
    globalTypingSettings().readSettings();
}

} // namespace TextEditor
