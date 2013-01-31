/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QSCRIPTSYNTAXHIGHLIGHTER_H
#define QSCRIPTSYNTAXHIGHLIGHTER_H

#include "qmljseditor_global.h"

#include <qmljs/qmljsscanner.h>

#include <QVector>
#include <QSet>
#include <QSyntaxHighlighter>

#include <texteditor/basetextdocumentlayout.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/texteditorconstants.h>

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
