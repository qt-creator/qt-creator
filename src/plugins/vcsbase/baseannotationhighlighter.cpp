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

#include "baseannotationhighlighter.h"

#include <math.h>
#include <QtCore/QSet>
#include <QtCore/QDebug>
#include <QtGui/QColor>
#include <QtGui/QTextDocument>
#include <QtGui/QTextEdit>
#include <QtGui/QTextCharFormat>

typedef QMap<QString, QTextCharFormat> ChangeNumberFormatMap;

namespace VCSBase {

struct BaseAnnotationHighlighterPrivate {
    ChangeNumberFormatMap m_changeNumberMap;
};

BaseAnnotationHighlighter::BaseAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                             QTextDocument *document) :
    QSyntaxHighlighter(document),
    m_d(new BaseAnnotationHighlighterPrivate)
{
    setChangeNumbers(changeNumbers);
}

BaseAnnotationHighlighter::~BaseAnnotationHighlighter()
{
    delete m_d;
}

void BaseAnnotationHighlighter::setChangeNumbers(const ChangeNumbers &changeNumbers)
{
    m_d->m_changeNumberMap.clear();
    if (!changeNumbers.isEmpty()) {
        // Assign a color gradient to annotation change numbers. Give
        // each change number a unique color.
        const double oneThird = 1.0 / 3.0;
        const int step = qRound(ceil(pow(double(changeNumbers.count()), oneThird)));
        QList<QColor> colors;
        const int factor = 255 / step;
        for (int i=0; i<step; ++i)
            for (int j=0; j<step; ++j)
                for (int k=0; k<step; ++k)
                    colors.append(QColor(i*factor, j*factor, k*factor));

        int m = 0;
        const int cstep = colors.count() / changeNumbers.count();
        const ChangeNumbers::const_iterator cend = changeNumbers.constEnd();
        for (ChangeNumbers::const_iterator it =  changeNumbers.constBegin(); it != cend; ++it) {
            QTextCharFormat format;
            format.setForeground(colors.at(m));
            m_d->m_changeNumberMap.insert(*it, format);
            m += cstep;
        }
    }
}

void BaseAnnotationHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty() || m_d->m_changeNumberMap.empty())
        return;
    const QString change = changeNumber(text);
    const ChangeNumberFormatMap::const_iterator it = m_d->m_changeNumberMap.constFind(change);
    if (it != m_d->m_changeNumberMap.constEnd())
        setFormat(0, text.length(), it.value());
}

} // namespace VCSBase
