/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSHIGHLIGHTER_H
#define QMLJSHIGHLIGHTER_H

#include "qmljseditor_global.h"

#include <qmljs/qmljsscanner.h>

#include <texteditor/textdocumentlayout.h>
#include <texteditor/syntaxhighlighter.h>

namespace QmlJSEditor {

class QMLJSEDITOR_EXPORT QmlJSHighlighter : public TextEditor::SyntaxHighlighter
{
    Q_OBJECT

public:
    QmlJSHighlighter(QTextDocument *parent = 0);
    virtual ~QmlJSHighlighter();

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
    bool m_qmlEnabled;
    int m_braceDepth;
    int m_foldingIndent;
    bool m_inMultilineComment;

    QmlJS::Scanner m_scanner;
    TextEditor::Parentheses m_currentBlockParentheses;
};

} // namespace QmlJSEditor

#endif // QMLJSHIGHLIGHTER_H
