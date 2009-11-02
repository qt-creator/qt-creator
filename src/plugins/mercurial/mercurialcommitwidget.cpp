/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#include "mercurialcommitwidget.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>

#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QTextEdit>

#include <QtCore/QDebug>
#include <QtCore/QRegExp>

//see the git submit widget for details of the syntax Highlighter

//TODO Check to see when the Highlighter has been moved to a base class and use that instead

namespace Mercurial {
namespace Internal {

// Retrieve the comment char format from the text editor.
static QTextCharFormat commentFormat()
{
    const TextEditor::FontSettings settings = TextEditor::TextEditorSettings::instance()->fontSettings();
    return settings.toTextCharFormat(QLatin1String(TextEditor::Constants::C_COMMENT));
}

// Highlighter for Mercurial submit messages. Make the first line bold, indicates
// comments as such (retrieving the format from the text editor) and marks up
// keywords (words in front of a colon as in 'Task: <bla>').
class MercurialSubmitHighlighter : QSyntaxHighlighter
{
public:
    explicit MercurialSubmitHighlighter(QTextEdit *parent);
    virtual void highlightBlock(const QString &text);

private:
    enum State { Header, Comment, Other };
    const QTextCharFormat m_commentFormat;
    const QRegExp m_keywordPattern;
    const QChar m_hashChar;
};

MercurialSubmitHighlighter::MercurialSubmitHighlighter(QTextEdit * parent) :
        QSyntaxHighlighter(parent),
        m_commentFormat(commentFormat()),
        m_keywordPattern(QLatin1String("^\\w+:")),
        m_hashChar(QLatin1Char('#'))
{
    Q_ASSERT(m_keywordPattern.isValid());
}

void MercurialSubmitHighlighter::highlightBlock(const QString &text)
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


MercurialCommitWidget::MercurialCommitWidget(QWidget *parent) :
        Utils::SubmitEditorWidget(parent),
        mercurialCommitPanel(new QWidget)
{
    mercurialCommitPanelUi.setupUi(mercurialCommitPanel);
    insertTopWidget(mercurialCommitPanel);
    new MercurialSubmitHighlighter(descriptionEdit());
}

void MercurialCommitWidget::setFields(const QString &repositoryRoot, const QString &branch,
                                      const QString &userName, const QString &email)
{
    mercurialCommitPanelUi.repositoryLabel->setText(repositoryRoot);
    mercurialCommitPanelUi.branchLabel->setText(branch);
    mercurialCommitPanelUi.authorLineEdit->setText(userName);
    mercurialCommitPanelUi.emailLineEdit->setText(email);
}

QString MercurialCommitWidget::committer()
{
    QString user = mercurialCommitPanelUi.authorLineEdit->text() + QLatin1String(" <") +
                   mercurialCommitPanelUi.emailLineEdit->text() + QLatin1String(">");
    return user;
}

QString MercurialCommitWidget::repoRoot()
{
    return mercurialCommitPanelUi.repositoryLabel->text();
}

} // namespace Internal
} // namespace Mercurial
