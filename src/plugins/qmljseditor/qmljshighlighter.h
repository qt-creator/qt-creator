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

#ifndef QSCRIPTSYNTAXHIGHLIGHTER_H
#define QSCRIPTSYNTAXHIGHLIGHTER_H

#include "qmljseditor_global.h"

#include <qmljs/qmljsscanner.h>

#include <QtCore/QVector>
#include <QtCore/QSet>
#include <QtGui/QSyntaxHighlighter>

#include <texteditor/basetextdocumentlayout.h>
#include <texteditor/syntaxhighlighter.h>

namespace QmlJSEditor {

class QMLJSEDITOR_EXPORT Highlighter : public TextEditor::SyntaxHighlighter
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
        FieldFormat,
        CommentFormat,
        VisualWhitespace,
        NumFormats
    };

    bool isQmlEnabled() const;
    void setQmlEnabled(bool duiEnabled);
    void setFormats(const QVector<QTextCharFormat> &formats);

protected:
    virtual void highlightBlock(const QString &text);

    int onBlockStart();
    void onBlockEnd(int state);

    // The functions are notified whenever parentheses are encountered.
    // Custom behaviour can be added, for example storing info for indenting.
    void onOpeningParenthesis(QChar parenthesis, int pos, bool atStart);
    void onClosingParenthesis(QChar parenthesis, int pos, bool atEnd);

    bool maybeQmlKeyword(const QStringRef &text) const;
    bool maybeQmlBuiltinType(const QStringRef &text) const;

private:
    typedef TextEditor::Parenthesis Parenthesis;
    typedef TextEditor::Parentheses Parentheses;

    bool m_qmlEnabled;
    int m_braceDepth;
    int m_foldingIndent;
    bool m_inMultilineComment;

    QmlJS::Scanner m_scanner;
    Parentheses m_currentBlockParentheses;

    QTextCharFormat m_formats[NumFormats];
};

} // namespace QmlJSEditor

#endif // QSCRIPTSYNTAXHIGHLIGHTER_H
