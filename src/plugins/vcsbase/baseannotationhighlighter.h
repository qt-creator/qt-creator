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

#ifndef BASEANNOTATIONHIGHLIGHTER_H
#define BASEANNOTATIONHIGHLIGHTER_H

#include "vcsbase_global.h"

#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QTextCharFormat>

namespace VCSBase {

struct BaseAnnotationHighlighterPrivate;

// Base for a highlighter for annotation lines of the form
// 'changenumber:XXXX'. The change numbers are assigned a color gradient.
// Example:
// 112: text1 <color 1>
// 113: text2 <color 2>
// 112: text3 <color 1>
class VCSBASE_EXPORT BaseAnnotationHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    typedef  QSet<QString> ChangeNumbers;

    explicit BaseAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                       QTextDocument *document = 0);
    virtual ~BaseAnnotationHighlighter();

    void setChangeNumbers(const ChangeNumbers &changeNumbers);

    virtual void highlightBlock(const QString &text);

private:
    // Implement this to return the change number of a line
    virtual QString changeNumber(const QString &block) const = 0;

    BaseAnnotationHighlighterPrivate *m_d;
};

} // namespace VCSBase

#endif // BASEANNOTATIONHIGHLIGHTER_H
