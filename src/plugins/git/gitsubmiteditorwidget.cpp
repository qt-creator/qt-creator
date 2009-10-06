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

#include "gitsubmiteditorwidget.h"
#include "commitdata.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>

#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QTextEdit>

#include <QtCore/QDebug>
#include <QtCore/QRegExp>

namespace Git {
namespace Internal {

// Retrieve the comment char format from the text editor.
static QTextCharFormat commentFormat()
{
    const TextEditor::FontSettings settings = TextEditor::TextEditorSettings::instance()->fontSettings();
    return settings.toTextCharFormat(QLatin1String(TextEditor::Constants::C_COMMENT));
}

// Highlighter for git submit messages. Make the first line bold, indicates
// comments as such (retrieving the format from the text editor) and marks up
// keywords (words in front of a colon as in 'Task: <bla>').

class GitSubmitHighlighter : QSyntaxHighlighter {
public:    
    explicit GitSubmitHighlighter(QTextEdit *parent);
    virtual void highlightBlock(const QString &text);

private:
    enum State { Header, Comment, Other };
    const QTextCharFormat m_commentFormat;    
    const QRegExp m_keywordPattern;
    const QChar m_hashChar;
};

GitSubmitHighlighter::GitSubmitHighlighter(QTextEdit * parent) :
    QSyntaxHighlighter(parent),
    m_commentFormat(commentFormat()),
    m_keywordPattern(QLatin1String("^\\w+:")),
    m_hashChar(QLatin1Char('#'))
{
    Q_ASSERT(m_keywordPattern.isValid());
}

void GitSubmitHighlighter::highlightBlock(const QString &text)
{
    // figure out current state
    State state = Other;
    const QTextBlock block = currentBlock();
    if (block.position() == 0) {
        state = Header;
    } else {
        if (text.startsWith(m_hashChar))
            state = Comment;
    }
    // Apply format.
    switch (state) {        
    case Header: {
            QTextCharFormat charFormat = format(0);
            charFormat.setFontWeight(QFont::Bold);
            setFormat(0, text.size(), charFormat);
    }
        break;
    case Comment:
        setFormat(0, text.size(), m_commentFormat);
        break;
    case Other:
        // Format key words ("Task:") italic
        if (m_keywordPattern.indexIn(text, 0, QRegExp::CaretAtZero) == 0) {
            QTextCharFormat charFormat = format(0);
            charFormat.setFontItalic(true);
            setFormat(0, m_keywordPattern.matchedLength(), charFormat);
        }
        break;
    }
}

// ------------------
GitSubmitEditorWidget::GitSubmitEditorWidget(QWidget *parent) :
    Utils::SubmitEditorWidget(parent),
    m_gitSubmitPanel(new QWidget)
{
    m_gitSubmitPanelUi.setupUi(m_gitSubmitPanel);
    insertTopWidget(m_gitSubmitPanel);
    new GitSubmitHighlighter(descriptionEdit());
}

void GitSubmitEditorWidget::setPanelInfo(const GitSubmitEditorPanelInfo &info)
{
    m_gitSubmitPanelUi.repositoryLabel->setText(info.repository);
    m_gitSubmitPanelUi.branchLabel->setText(info.branch);
}

GitSubmitEditorPanelData GitSubmitEditorWidget::panelData() const
{
    GitSubmitEditorPanelData rc;
    rc.author = m_gitSubmitPanelUi.authorLineEdit->text();
    rc.email = m_gitSubmitPanelUi.emailLineEdit->text();
    return rc;
}

void GitSubmitEditorWidget::setPanelData(const GitSubmitEditorPanelData &data)
{
    m_gitSubmitPanelUi.authorLineEdit->setText(data.author);
    m_gitSubmitPanelUi.emailLineEdit->setText(data.email);
}

} // namespace Internal
} // namespace Git
