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

#include "tabsettings.h"

#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>

static const char *spacesForTabsKey = "SpacesForTabs";
static const char *smartBackspaceKey = "SmartBackspace";
static const char *autoIndentKey = "AutoIndent";
static const char *tabSizeKey = "TabSize";
static const char *indentSizeKey = "IndentSize";
static const char *groupPostfix = "TabSettings";

namespace TextEditor {

TabSettings::TabSettings() :
    m_spacesForTabs(true),
    m_autoIndent(true),
    m_smartBackspace(false),
    m_tabSize(8),
    m_indentSize(4)
{
}

void TabSettings::toSettings(const QString &category, QSettings *s) const
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);
    s->beginGroup(group);
    s->setValue(QLatin1String(spacesForTabsKey),  m_spacesForTabs);
    s->setValue(QLatin1String(autoIndentKey), m_autoIndent);
    s->setValue(QLatin1String(smartBackspaceKey), m_smartBackspace);
    s->setValue(QLatin1String(tabSizeKey), m_tabSize);
    s->setValue(QLatin1String(indentSizeKey), m_indentSize);
    s->endGroup();
}

void TabSettings::fromSettings(const QString &category, const QSettings *s)
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);
    group += QLatin1Char('/');

    *this = TabSettings(); // Assign defaults

    m_spacesForTabs   = s->value(group + QLatin1String(spacesForTabsKey), m_spacesForTabs).toBool();
    m_autoIndent      = s->value(group + QLatin1String(autoIndentKey), m_autoIndent).toBool();
    m_smartBackspace  = s->value(group + QLatin1String(smartBackspaceKey), m_smartBackspace).toBool();
    m_tabSize         = s->value(group + QLatin1String(tabSizeKey), m_tabSize).toInt();
    m_indentSize      = s->value(group + QLatin1String(indentSizeKey), m_indentSize).toInt();
}

int TabSettings::lineIndentPosition(const QString &text) const
{
    int i = 0;
    while (i < text.size()) {
        if (!text.at(i).isSpace())
            break;
        ++i;
    }
    int column = columnAt(text, i);
    return i - (column % m_indentSize);
}

int TabSettings::firstNonSpace(const QString &text) const
{
    int i = 0;
    while (i < text.size()) {
        if (!text.at(i).isSpace())
            return i;
        ++i;
    }
    return i;
}

int TabSettings::trailingWhitespaces(const QString &text) const
{
    int i = 0;
    while (i < text.size()) {
        if (!text.at(text.size()-1-i).isSpace())
            return i;
        ++i;
    }
    return i;
}

bool TabSettings::isIndentationClean(const QString &text) const
{
    int i = 0;
    int spaceCount = 0;
    while (i < text.size()) {
        QChar c = text.at(i);
        if (!c.isSpace())
            return true;

        if (c == QLatin1Char(' ')) {
            ++spaceCount;
            if (!m_spacesForTabs && spaceCount == m_tabSize)
                return false;
        } else if (c == QLatin1Char('\t')) {
            if (m_spacesForTabs || spaceCount != m_indentSize)
                return false;
            spaceCount = 0;
        }
        ++i;
    }
    return true;
}

int TabSettings::columnAt(const QString &text, int position) const
{
    int column = 0;
    for (int i = 0; i < position; ++i) {
        if (text.at(i) == QLatin1Char('\t'))
            column = column - (column % m_tabSize) + m_tabSize;
        else
            ++column;
    }
    return column;
}

int TabSettings::spacesLeftFromPosition(const QString &text, int position) const
{
    int i = position;
    while (i > 0) {
        if (!text.at(i-1).isSpace())
            break;
        --i;
    }
    return position - i;
}

int TabSettings::indentedColumn(int column, bool doIndent) const
{
    int aligned = (column / m_indentSize) * m_indentSize;
    if (doIndent)
        return aligned + m_indentSize;
    if (aligned < column)
        return aligned;
    return qMax(0, aligned - m_indentSize);
}

QString TabSettings::indentationString(int startColumn, int targetColumn) const
{
    targetColumn = qMax(startColumn, targetColumn);
    if (m_spacesForTabs)
        return QString(targetColumn - startColumn, QLatin1Char(' '));

    QString s;
    int alignedStart = startColumn - (startColumn % m_tabSize) + m_tabSize;
    if (alignedStart > startColumn && alignedStart <= targetColumn) {
        s += QLatin1Char('\t');
        startColumn = alignedStart;
    }
    if (int columns = targetColumn - startColumn) {
        int tabs = columns / m_tabSize;
        s += QString(tabs, QLatin1Char('\t'));
        s += QString(columns - tabs * m_tabSize, QLatin1Char(' '));
    }
    return s;
}

void TabSettings::indentLine(QTextBlock block, int newIndent) const
{
    const QString text = block.text();
    const int oldBlockLength = text.size();

    // Quickly check whether indenting is required.
    if (oldBlockLength == 0 && newIndent == 0)
        return;

    const QString indentString = indentationString(0, newIndent);
    newIndent = indentString.length();

    if (oldBlockLength == indentString.length() && text == indentString)
        return;

    if (oldBlockLength > indentString.length() &&
        text.startsWith(indentString) &&
        !text.at(indentString.length()).isSpace()) {
        return;
    }

    QTextCursor cursor(block);
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, firstNonSpace(text));
    cursor.removeSelectedText();
    cursor.insertText(indentString);
    cursor.endEditBlock();
}

bool TabSettings::equals(const TabSettings &ts) const
{
    return m_spacesForTabs == ts.m_spacesForTabs
        && m_autoIndent == ts.m_autoIndent
        && m_smartBackspace == ts.m_smartBackspace
        && m_tabSize == ts.m_tabSize
        && m_indentSize == ts.m_indentSize;
}

} // namespace TextEditor
