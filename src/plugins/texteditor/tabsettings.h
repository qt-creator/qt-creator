/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef TABSETTINGS_H
#define TABSETTINGS_H

#include "texteditor_global.h"

#include <QtGui/QTextBlock>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

// Tab settings: Data type the GeneralSettingsPage acts on
// with some convenience functions for formatting.
struct TEXTEDITOR_EXPORT TabSettings
{
    TabSettings();

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);


    int lineIndentPosition(const QString &text) const;
    int firstNonSpace(const QString &text) const;
    int columnAt(const QString &text, int position) const;
    int spacesLeftFromPosition(const QString &text, int position) const;
    int indentedColumn(int column, bool doIndent = true) const;
    QString indentationString(int startColumn, int targetColumn) const;

    void indentLine(QTextBlock block, int newIndent) const;

    int trailingWhitespaces(const QString &text) const;
    bool isIndentationClean(const QString &text) const;


    bool m_spacesForTabs;
    bool m_autoIndent;
    bool m_smartBackspace;
    int m_tabSize;
    int m_indentSize;

    bool equals(const TabSettings &ts) const;
};

inline bool operator==(const TabSettings &t1, const TabSettings &t2) { return t1.equals(t2); }
inline bool operator!=(const TabSettings &t1, const TabSettings &t2) { return !t1.equals(t2); }

} // namespace TextEditor

#endif // TABSETTINGS_H
