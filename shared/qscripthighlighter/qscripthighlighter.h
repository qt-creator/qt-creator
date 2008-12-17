/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef QSCRIPTSYNTAXHIGHLIGHTER_H
#define QSCRIPTSYNTAXHIGHLIGHTER_H

#include <QVector>
#include <QtGui/QSyntaxHighlighter>

namespace SharedTools {

class QScriptHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    QScriptHighlighter(QTextDocument *parent = 0);
    virtual void highlightBlock(const QString &text);

    enum { NumberFormat, StringFormat, TypeFormat,
           KeywordFormat, PreProcessorFormat, LabelFormat, CommentFormat,
           NumFormats };

    // MS VC 6 compatible, still.
    void setFormats(const QVector<QTextCharFormat> &s);
    static const QVector<QTextCharFormat> &defaultFormats();

private:
    // The functions are notified whenever parentheses are encountered.
    // Custom behaviour can be added, for example storing info for indenting.
    virtual int onBlockStart(); // returns the blocks initial state
    virtual void onOpeningParenthesis(QChar parenthesis, int pos);
    virtual void onClosingParenthesis(QChar parenthesis, int pos);
    // sets the enriched user state, or simply calls setCurrentBlockState(state);
    virtual void onBlockEnd(int state, int firstNonSpace);

    void highlightKeyword(int currentPos, const QString &buffer);

    QTextCharFormat m_formats[NumFormats];
};

} // namespace SharedTools

#endif // QSCRIPTSYNTAXHIGHLIGHTER_H
