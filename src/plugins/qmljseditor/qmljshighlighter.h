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

#include <texteditor/basetexteditor.h>

namespace QmlJSEditor {
namespace Internal {

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(QTextDocument *parent = 0);
    virtual ~Highlighter();

    enum {
        NumberFormat,
        StringFormat,
        TypeFormat,
        KeywordFormat,
        LabelFormat,
        CommentFormat,
        VisualWhitespace,
        NumFormats
    };

    bool isQmlEnabled() const;
    void setQmlEnabled(bool duiEnabled);

    // MS VC 6 compatible, still.
    // Set formats from a sequence of type QTextCharFormat
    template <class InputIterator>
    void setFormats(InputIterator begin, InputIterator end)
    {
        qCopy(begin, end, m_formats);
    }

    QTextCharFormat labelTextCharFormat() const;

protected:
    virtual void highlightBlock(const QString &text);

    int onBlockStart();
    void onBlockEnd(int state, int firstNonSpace);

    // The functions are notified whenever parentheses are encountered.
    // Custom behaviour can be added, for example storing info for indenting.
    void onOpeningParenthesis(QChar parenthesis, int pos);
    void onClosingParenthesis(QChar parenthesis, int pos);

    bool maybeQmlKeyword(const QStringRef &text) const;
    bool maybeQmlBuiltinType(const QStringRef &text) const;

private:
    typedef TextEditor::Parenthesis Parenthesis;
    typedef TextEditor::Parentheses Parentheses;

    bool m_qmlEnabled;
    int m_braceDepth;

    QmlJS::Scanner m_scanner;
    Parentheses m_currentBlockParentheses;

    QTextCharFormat m_formats[NumFormats];
};

} // end of namespace Internal
} // end of namespace QmlJSEditor

#endif // QSCRIPTSYNTAXHIGHLIGHTER_H
