/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "displaysettings.h"

#include <QSettings>
#include <QString>

static const char displayLineNumbersKey[] = "DisplayLineNumbers";
static const char textWrappingKey[] = "TextWrapping";
static const char showWrapColumnKey[] = "ShowWrapColumn";
static const char wrapColumnKey[] = "WrapColumn";
static const char visualizeWhitespaceKey[] = "VisualizeWhitespace";
static const char displayFoldingMarkersKey[] = "DisplayFoldingMarkers";
static const char highlightCurrentLineKey[] = "HighlightCurrentLine2Key";
static const char highlightBlocksKey[] = "HighlightBlocksKey";
static const char animateMatchingParenthesesKey[] = "AnimateMatchingParenthesesKey";
static const char highlightMatchingParenthesesKey[] = "HightlightMatchingParenthesesKey";
static const char markTextChangesKey[] = "MarkTextChanges";
static const char autoFoldFirstCommentKey[] = "AutoFoldFirstComment";
static const char centerCursorOnScrollKey[] = "CenterCursorOnScroll";
static const char openLinksInNextSplitKey[] = "OpenLinksInNextSplitKey";
static const char forceOpenLinksInNextSplitKey[] = "ForceOpenLinksInNextSplitKey";
static const char groupPostfix[] = "DisplaySettings";

namespace TextEditor {

DisplaySettings::DisplaySettings() :
    m_displayLineNumbers(true),
    m_textWrapping(false),
    m_showWrapColumn(false),
    m_wrapColumn(80),
    m_visualizeWhitespace(false),
    m_displayFoldingMarkers(true),
    m_highlightCurrentLine(false),
    m_highlightBlocks(false),
    m_animateMatchingParentheses(true),
    m_highlightMatchingParentheses(true),
    m_markTextChanges(true),
    m_autoFoldFirstComment(true),
    m_centerCursorOnScroll(false),
    m_openLinksInNextSplit(true),
    m_forceOpenLinksInNextSplit(false)
{
}

void DisplaySettings::toSettings(const QString &category, QSettings *s) const
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);
    s->beginGroup(group);
    s->setValue(QLatin1String(displayLineNumbersKey), m_displayLineNumbers);
    s->setValue(QLatin1String(textWrappingKey), m_textWrapping);
    s->setValue(QLatin1String(showWrapColumnKey), m_showWrapColumn);
    s->setValue(QLatin1String(wrapColumnKey), m_wrapColumn);
    s->setValue(QLatin1String(visualizeWhitespaceKey), m_visualizeWhitespace);
    s->setValue(QLatin1String(displayFoldingMarkersKey), m_displayFoldingMarkers);
    s->setValue(QLatin1String(highlightCurrentLineKey), m_highlightCurrentLine);
    s->setValue(QLatin1String(highlightBlocksKey), m_highlightBlocks);
    s->setValue(QLatin1String(animateMatchingParenthesesKey), m_animateMatchingParentheses);
    s->setValue(QLatin1String(highlightMatchingParenthesesKey), m_highlightMatchingParentheses);
    s->setValue(QLatin1String(markTextChangesKey), m_markTextChanges);
    s->setValue(QLatin1String(autoFoldFirstCommentKey), m_autoFoldFirstComment);
    s->setValue(QLatin1String(centerCursorOnScrollKey), m_centerCursorOnScroll);
    s->setValue(QLatin1String(openLinksInNextSplitKey), m_openLinksInNextSplit);
    s->setValue(QLatin1String(forceOpenLinksInNextSplitKey), m_forceOpenLinksInNextSplit);
    s->endGroup();
}

void DisplaySettings::fromSettings(const QString &category, const QSettings *s)
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);
    group += QLatin1Char('/');

    *this = DisplaySettings(); // Assign defaults

    m_displayLineNumbers = s->value(group + QLatin1String(displayLineNumbersKey), m_displayLineNumbers).toBool();
    m_textWrapping = s->value(group + QLatin1String(textWrappingKey), m_textWrapping).toBool();
    m_showWrapColumn = s->value(group + QLatin1String(showWrapColumnKey), m_showWrapColumn).toBool();
    m_wrapColumn = s->value(group + QLatin1String(wrapColumnKey), m_wrapColumn).toInt();
    m_visualizeWhitespace = s->value(group + QLatin1String(visualizeWhitespaceKey), m_visualizeWhitespace).toBool();
    m_displayFoldingMarkers = s->value(group + QLatin1String(displayFoldingMarkersKey), m_displayFoldingMarkers).toBool();
    m_highlightCurrentLine = s->value(group + QLatin1String(highlightCurrentLineKey), m_highlightCurrentLine).toBool();
    m_highlightBlocks = s->value(group + QLatin1String(highlightBlocksKey), m_highlightBlocks).toBool();
    m_animateMatchingParentheses = s->value(group + QLatin1String(animateMatchingParenthesesKey), m_animateMatchingParentheses).toBool();
    m_highlightMatchingParentheses = s->value(group + QLatin1String(highlightMatchingParenthesesKey), m_highlightMatchingParentheses).toBool();
    m_markTextChanges = s->value(group + QLatin1String(markTextChangesKey), m_markTextChanges).toBool();
    m_autoFoldFirstComment = s->value(group + QLatin1String(autoFoldFirstCommentKey), m_autoFoldFirstComment).toBool();
    m_centerCursorOnScroll = s->value(group + QLatin1String(centerCursorOnScrollKey), m_centerCursorOnScroll).toBool();
    m_openLinksInNextSplit = s->value(group + QLatin1String(openLinksInNextSplitKey), m_openLinksInNextSplit).toBool();
    m_forceOpenLinksInNextSplit = s->value(group + QLatin1String(forceOpenLinksInNextSplitKey), m_forceOpenLinksInNextSplit).toBool();
}

bool DisplaySettings::equals(const DisplaySettings &ds) const
{
    return m_displayLineNumbers == ds.m_displayLineNumbers
        && m_textWrapping == ds.m_textWrapping
        && m_showWrapColumn == ds.m_showWrapColumn
        && m_wrapColumn == ds.m_wrapColumn
        && m_visualizeWhitespace == ds.m_visualizeWhitespace
        && m_displayFoldingMarkers == ds.m_displayFoldingMarkers
        && m_highlightCurrentLine == ds.m_highlightCurrentLine
        && m_highlightBlocks == ds.m_highlightBlocks
        && m_animateMatchingParentheses == ds.m_animateMatchingParentheses
        && m_highlightMatchingParentheses == ds.m_highlightMatchingParentheses
        && m_markTextChanges == ds.m_markTextChanges
        && m_autoFoldFirstComment== ds.m_autoFoldFirstComment
        && m_centerCursorOnScroll == ds.m_centerCursorOnScroll
        && m_openLinksInNextSplit == ds.m_openLinksInNextSplit
        && m_forceOpenLinksInNextSplit == ds.m_forceOpenLinksInNextSplit
        ;
}

} // namespace TextEditor
