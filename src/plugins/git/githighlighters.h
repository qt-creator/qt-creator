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

#ifndef GITHIGHLIGHTERS_H
#define GITHIGHLIGHTERS_H

#include <texteditor/syntaxhighlighter.h>
#include <texteditor/texteditorconstants.h>

namespace TextEditor {
class FontSettings;
}

namespace Git {
namespace Internal {

// Highlighter for git submit messages. Make the first line bold, indicates
// comments as such (retrieving the format from the text editor) and marks up
// keywords (words in front of a colon as in 'Task: <bla>').
class GitSubmitHighlighter : public TextEditor::SyntaxHighlighter
{
public:
    explicit GitSubmitHighlighter(QTextEdit *parent);
    explicit GitSubmitHighlighter(TextEditor::BaseTextDocument *parent);
    void highlightBlock(const QString &text);

    void initialize();
private:
    enum State { None = -1, Header, Other };
    QTextCharFormat m_commentFormat;
    QRegExp m_keywordPattern;
    QChar m_hashChar;
};

// Highlighter for interactive rebase todo. Indicates comments as such
// (retrieving the format from the text editor) and marks up keywords
class GitRebaseHighlighter : public TextEditor::SyntaxHighlighter
{
public:
    explicit GitRebaseHighlighter(TextEditor::BaseTextDocument *parent);
    void highlightBlock(const QString &text);

private:
    class RebaseAction
    {
    public:
        mutable QRegExp exp;
        QTextCharFormat format;
        RebaseAction(const QString &regexp, const TextEditor::FontSettings &settings,
                     TextEditor::TextStyle category);
    };
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_changeFormat;
    QTextCharFormat m_descFormat;
    const QChar m_hashChar;
    QRegExp m_changeNumberPattern;
    QList<RebaseAction> m_actions;
};

} // namespace Internal
} // namespace Git

#endif // GITHIGHLIGHTERS_H
