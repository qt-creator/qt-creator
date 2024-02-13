// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baseannotationhighlighter.h"

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/algorithm.h>

#include <QColor>
#include <QDebug>
#include <QTextCharFormat>
#include <QTextDocument>

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
    QSet<QString> annotationChanges() const;

    ChangeNumberFormatMap m_changeNumberMap;
    Annotation m_annotation;
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


QSet<QString> BaseAnnotationHighlighterPrivate::annotationChanges() const
{
    if (!q->document())
        return {};

    QSet<QString> changes;
    const QString text = q->document()->toPlainText();
    QStringView txt = QStringView(text);
    if (txt.isEmpty())
        return changes;
    if (!m_annotation.separatorPattern.pattern().isEmpty()) {
        const QRegularExpressionMatch match = m_annotation.separatorPattern.match(txt);
        if (match.hasMatch())
            txt.truncate(match.capturedStart());
    }
    QRegularExpressionMatchIterator i = m_annotation.entryPattern.globalMatch(txt);
    while (i.hasNext()) {
        const QRegularExpressionMatch match = i.next();
        changes.insert(match.captured(1));
    }
    return changes;
}

BaseAnnotationHighlighter::BaseAnnotationHighlighter(const Annotation &annotation)
    : TextEditor::SyntaxHighlighter()
    , d(new BaseAnnotationHighlighterPrivate(this))
{
    setDefaultTextFormatCategories();

    d->m_annotation = annotation;
    d->updateOtherFormats();
}

BaseAnnotationHighlighter::~BaseAnnotationHighlighter()
{
    // The destructor of the base class indirectly triggers a QTextDocument::contentsChang signal
    // which is still connected to setChangeNumbersForAnnotation. The connection is guarded by
    // 'this', but since it was not fully destructed it still calls the function. Explicitly set
    // a null document here to disconnect that connection and correctly reset all highlights.
    setDocument(nullptr);
    delete d;
}

void BaseAnnotationHighlighter::documentChanged(QTextDocument *oldDoc, QTextDocument *newDoc)
{
    if (oldDoc)
        disconnect(oldDoc,
                   &QTextDocument::contentsChange,
                   this,
                   &BaseAnnotationHighlighter::setChangeNumbersForAnnotation);

    if (newDoc)
        connect(newDoc,
                &QTextDocument::contentsChange,
                this,
                &BaseAnnotationHighlighter::setChangeNumbersForAnnotation);
}

void BaseAnnotationHighlighter::setChangeNumbersForAnnotation()
{
    setChangeNumbers(d->annotationChanges());
    d->updateOtherFormats();
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

void BaseAnnotationHighlighter::rehighlight()
{
    const QSet<QString> changes = d->annotationChanges();
    if (changes.isEmpty())
        return;

    setChangeNumbers(changes);
    TextEditor::SyntaxHighlighter::rehighlight();
}

} // namespace VcsBase
