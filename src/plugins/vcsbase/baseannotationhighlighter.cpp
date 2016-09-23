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

#include "baseannotationhighlighter.h"
#include <texteditor/fontsettings.h>

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
    enum Formats {
        BackgroundFormat // C_TEXT
    };

    BaseAnnotationHighlighterPrivate(BaseAnnotationHighlighter *q_) : q(q_) { }

    void updateOtherFormats();

    ChangeNumberFormatMap m_changeNumberMap;
    QColor m_background;
    BaseAnnotationHighlighter *const q;
};

void BaseAnnotationHighlighterPrivate::updateOtherFormats()
{
    m_background = q->formatForCategory(BackgroundFormat).brushProperty(QTextFormat::BackgroundBrush).color();
    q->setChangeNumbers(m_changeNumberMap.keys().toSet());
}

BaseAnnotationHighlighter::BaseAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                                     QTextDocument *document) :
    TextEditor::SyntaxHighlighter(document),
    d(new BaseAnnotationHighlighterPrivate(this))
{
    static const QVector<TextEditor::TextStyle> categories({TextEditor::C_TEXT});

    setTextFormatCategories(categories);
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
    if (!changeNumbers.isEmpty()) {
        // Assign a color gradient to annotation change numbers. Give
        // each change number a unique color.
        const QList<QColor> colors =
            TextEditor::SyntaxHighlighter::generateColors(changeNumbers.size(), d->m_background);
        int m = 0;
        const int cstep = colors.count() / changeNumbers.count();
        const ChangeNumbers::const_iterator cend = changeNumbers.constEnd();
        for (ChangeNumbers::const_iterator it =  changeNumbers.constBegin(); it != cend; ++it) {
            QTextCharFormat format;
            format.setForeground(colors.at(m));
            d->m_changeNumberMap.insert(*it, format);
            m += cstep;
        }
    }
}

void BaseAnnotationHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty() || d->m_changeNumberMap.empty())
        return;
    const QString change = changeNumber(text);
    const ChangeNumberFormatMap::const_iterator it = d->m_changeNumberMap.constFind(change);
    if (it != d->m_changeNumberMap.constEnd())
        setFormat(0, text.length(), it.value());
}

void BaseAnnotationHighlighter::setFontSettings(const TextEditor::FontSettings &fontSettings)
{
    SyntaxHighlighter::setFontSettings(fontSettings);
    d->updateOtherFormats();
}

} // namespace VcsBase
