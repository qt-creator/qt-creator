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

#ifndef GITHIGHLIGHTERS_H
#define GITHIGHLIGHTERS_H

#include <texteditor/syntaxhighlighter.h>

namespace Git {
namespace Internal {

enum Format {
    Format_Comment,
    Format_Change,
    Format_Description,
    Format_Pick,
    Format_Reword,
    Format_Edit,
    Format_Squash,
    Format_Fixup,
    Format_Exec
};

// Highlighter for git submit messages. Make the first line bold, indicates
// comments as such (retrieving the format from the text editor) and marks up
// keywords (words in front of a colon as in 'Task: <bla>').
class GitSubmitHighlighter : public TextEditor::SyntaxHighlighter
{
public:
    explicit GitSubmitHighlighter(QTextEdit *parent = 0);
    void highlightBlock(const QString &text) override;

private:
    enum State { None = -1, Header, Other };
    QRegExp m_keywordPattern;
    QChar m_hashChar;
};

// Highlighter for interactive rebase todo. Indicates comments as such
// (retrieving the format from the text editor) and marks up keywords
class GitRebaseHighlighter : public TextEditor::SyntaxHighlighter
{
public:
    explicit GitRebaseHighlighter(QTextDocument *parent = 0);
    void highlightBlock(const QString &text) override;

private:
    class RebaseAction
    {
    public:
        mutable QRegExp exp;
        Format formatCategory;
        RebaseAction(const QString &regexp, const Format formatCategory);
    };
    const QChar m_hashChar;
    QRegExp m_changeNumberPattern;
    QList<RebaseAction> m_actions;
};

} // namespace Internal
} // namespace Git

#endif // GITHIGHLIGHTERS_H
