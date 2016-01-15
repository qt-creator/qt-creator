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

#include "displaysettings.h"

#include <QSettings>
#include <QString>

static const char displayLineNumbersKey[] = "DisplayLineNumbers";
static const char textWrappingKey[] = "TextWrapping";
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
static const char displayFileEncodingKey[] = "DisplayFileEncoding";
static const char scrollBarHighlightsKey[] = "ScrollBarHighlights";
static const char groupPostfix[] = "DisplaySettings";

namespace TextEditor {

DisplaySettings::DisplaySettings() :
    m_displayLineNumbers(true),
    m_textWrapping(false),
    m_visualizeWhitespace(false),
    m_displayFoldingMarkers(true),
    m_highlightCurrentLine(false),
    m_highlightBlocks(false),
    m_animateMatchingParentheses(true),
    m_highlightMatchingParentheses(true),
    m_markTextChanges(true),
    m_autoFoldFirstComment(true),
    m_centerCursorOnScroll(false),
    m_openLinksInNextSplit(false),
    m_forceOpenLinksInNextSplit(false),
    m_displayFileEncoding(false),
    m_scrollBarHighlights(true)
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
    s->setValue(QLatin1String(displayFileEncodingKey), m_displayFileEncoding);
    s->setValue(QLatin1String(scrollBarHighlightsKey), m_scrollBarHighlights);
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
    m_displayFileEncoding = s->value(group + QLatin1String(displayFileEncodingKey), m_displayFileEncoding).toBool();
    m_scrollBarHighlights = s->value(group + QLatin1String(scrollBarHighlightsKey), m_scrollBarHighlights).toBool();
}

bool DisplaySettings::equals(const DisplaySettings &ds) const
{
    return m_displayLineNumbers == ds.m_displayLineNumbers
        && m_textWrapping == ds.m_textWrapping
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
        && m_displayFileEncoding == ds.m_displayFileEncoding
        && m_scrollBarHighlights == ds.m_scrollBarHighlights
        ;
}

} // namespace TextEditor
