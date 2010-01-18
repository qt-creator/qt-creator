/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QSCRIPTSYNTAXHIGHLIGHTER_H
#define QSCRIPTSYNTAXHIGHLIGHTER_H

#include <qmljs/qmljsscanner.h>

#include <QtCore/QVector>
#include <QtCore/QSet>
#include <QtGui/QSyntaxHighlighter>

namespace QmlJS {

class QMLJS_EXPORT QScriptHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    QScriptHighlighter(bool duiEnabled = false, QTextDocument *parent = 0);
    virtual void highlightBlock(const QString &text);

    enum { NumberFormat, StringFormat, TypeFormat,
           KeywordFormat, PreProcessorFormat, LabelFormat, CommentFormat,
           VisualWhitespace,
           NumFormats };

    bool isDuiEnabled() const;

    // MS VC 6 compatible, still.
    void setFormats(const QVector<QTextCharFormat> &s);

    QTextCharFormat labelTextCharFormat() const
    { return m_formats[LabelFormat]; }

    QSet<QString> keywords();

protected:
    // The functions are notified whenever parentheses are encountered.
    // Custom behaviour can be added, for example storing info for indenting.
    virtual int onBlockStart(); // returns the blocks initial state
    virtual void onOpeningParenthesis(QChar parenthesis, int pos);
    virtual void onClosingParenthesis(QChar parenthesis, int pos);
    // sets the enriched user state, or simply calls setCurrentBlockState(state);
    virtual void onBlockEnd(int state, int firstNonSpace);

    virtual void highlightWhitespace(const QmlJSScanner::Token &token, const QString &text, int nonWhitespaceFormat);

protected:
    QmlJSScanner m_scanner;

private:
    bool m_duiEnabled;
    QTextCharFormat m_formats[NumFormats];
};

} // namespace QmlJS

#endif // QSCRIPTSYNTAXHIGHLIGHTER_H
