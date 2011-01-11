/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef BASEANNOTATIONHIGHLIGHTER_H
#define BASEANNOTATIONHIGHLIGHTER_H

#include "vcsbase_global.h"
#include <texteditor/syntaxhighlighter.h>
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
class VCSBASE_EXPORT BaseAnnotationHighlighter : public TextEditor::SyntaxHighlighter
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
