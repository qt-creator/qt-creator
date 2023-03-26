// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baseannotationhighlighter.h"

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/algorithm.h>

#include <QDebug>
#include <QColor>
#include <QTextDocument>
#include <QTextCharFormat>

typedef QMap<QString, QTextCharFormat> ChangeNumberFormatMap;

/*!
    \class VcsBase::BaseAnnotationHighlighter

    \brief The BaseAnnotationHighlighter class is the base class for a
    highlighter for annotation lines of the form 'changenumber:XXXX'.

    The change numbers are assigned a color gradient. Example:
    \code
    112: text1 <color 1>
    113: text2 <color 2>
    112: text3 <color 1>
    \endcode
*/

namespace VcsBase {

class BaseAnnotationHighlighterPrivate
{
public:
    BaseAnnotationHighlighterPrivate(BaseAnnotationHighlighter *q_) : q(q_) { }

    void updateOtherFormats();

    ChangeNumberFormatMap m_changeNumberMap;
    QColor m_background;
    BaseAnnotationHighlighter *const q;
};

void BaseAnnotationHighlighterPrivate::updateOtherFormats()
{
    m_background = q->fontSettings()
                       .toTextCharFormat(TextEditor::C_TEXT)
                       .brushProperty(QTextFormat::BackgroundBrush)
                       .color();
    q->setChangeNumbers(Utils::toSet(m_changeNumberMap.keys()));
}

BaseAnnotationHighlighter::BaseAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                                     QTextDocument *document) :
    TextEditor::SyntaxHighlighter(document),
    d(new BaseAnnotationHighlighterPrivate(this))
{
    setDefaultTextFormatCategories();
    d->updateOtherFormats();

    setChangeNumbers(changeNumbers);
}

BaseAnnotationHighlighter::~BaseAnnotationHighlighter()
{
    delete d;
}

void BaseAnnotationHighlighter::setChangeNumbers(const ChangeNumbers &changeNumbers)
{
    d->m_changeNumberMap.clear();
    const int changeNumberCount = changeNumbers.size();
    if (changeNumberCount == 0)
        return;
    // Assign a color gradient to annotation change numbers. Give
    // each change number a unique color.
    const QList<QColor> colors =
        TextEditor::SyntaxHighlighter::generateColors(changeNumberCount, d->m_background);
    int m = 0;
    const int cstep = colors.count() / changeNumberCount;
    const ChangeNumbers::const_iterator cend = changeNumbers.constEnd();
    for (ChangeNumbers::const_iterator it =  changeNumbers.constBegin(); it != cend; ++it) {
        QTextCharFormat format;
        format.setForeground(colors.at(m));
        d->m_changeNumberMap.insert(*it, format);
        m += cstep;
    }
}

void BaseAnnotationHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty() || d->m_changeNumberMap.empty())
        return;
    const QString change = changeNumber(text);
    const ChangeNumberFormatMap::const_iterator it = d->m_changeNumberMap.constFind(change);
    if (it != d->m_changeNumberMap.constEnd())
        setFormatWithSpaces(text, 0, text.length(), it.value());
}

void BaseAnnotationHighlighter::setFontSettings(const TextEditor::FontSettings &fontSettings)
{
    SyntaxHighlighter::setFontSettings(fontSettings);
    d->updateOtherFormats();
}

} // namespace VcsBase
