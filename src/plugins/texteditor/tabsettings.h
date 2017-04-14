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

#pragma once

#include "texteditor_global.h"

#include <QTextBlock>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

// Tab settings: Data type the GeneralSettingsPage acts on
// with some convenience functions for formatting.
class TEXTEDITOR_EXPORT TabSettings
{
public:

    enum TabPolicy {
        SpacesOnlyTabPolicy = 0,
        TabsOnlyTabPolicy = 1,
        MixedTabPolicy = 2
    };

    // This enum must match the indexes of continuationAlignBehavior widget
    enum ContinuationAlignBehavior {
        NoContinuationAlign = 0,
        ContinuationAlignWithSpaces = 1,
        ContinuationAlignWithIndent = 2
    };

    TabSettings() = default;
    TabSettings(TabPolicy tabPolicy, int tabSize,
                int indentSize, ContinuationAlignBehavior continuationAlignBehavior);

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    void toMap(const QString &prefix, QVariantMap *map) const;
    void fromMap(const QString &prefix, const QVariantMap &map);

    int lineIndentPosition(const QString &text) const;
    int columnAt(const QString &text, int position) const;
    int positionAtColumn(const QString &text, int column, int *offset = 0, bool allowOverstep = false) const;
    int columnCountForText(const QString &text, int startColumn = 0) const;
    int indentedColumn(int column, bool doIndent = true) const;
    QString indentationString(int startColumn, int targetColumn, int padding, const QTextBlock &currentBlock = QTextBlock()) const;
    QString indentationString(const QString &text) const;
    int indentationColumn(const QString &text) const;
    static int maximumPadding(const QString &text);

    void indentLine(const QTextBlock &block, int newIndent, int padding = 0) const;
    void reindentLine(QTextBlock block, int delta) const;

    bool isIndentationClean(const QTextBlock &block, const int indent) const;
    bool guessSpacesForTabs(const QTextBlock &block) const;

    static int firstNonSpace(const QString &text);
    static inline bool onlySpace(const QString &text) { return firstNonSpace(text) == text.length(); }
    static int spacesLeftFromPosition(const QString &text, int position);
    static bool cursorIsAtBeginningOfLine(const QTextCursor &cursor);
    static int trailingWhitespaces(const QString &text);
    static void removeTrailingWhitespace(QTextCursor cursor, QTextBlock &block);

    TabPolicy m_tabPolicy = SpacesOnlyTabPolicy;
    int m_tabSize = 8;
    int m_indentSize = 4;
    ContinuationAlignBehavior m_continuationAlignBehavior = ContinuationAlignWithSpaces;

    bool equals(const TabSettings &ts) const;
};

inline bool operator==(const TabSettings &t1, const TabSettings &t2) { return t1.equals(t2); }
inline bool operator!=(const TabSettings &t1, const TabSettings &t2) { return !t1.equals(t2); }

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::TabSettings)
Q_DECLARE_METATYPE(TextEditor::TabSettings::TabPolicy)
Q_DECLARE_METATYPE(TextEditor::TabSettings::ContinuationAlignBehavior)
