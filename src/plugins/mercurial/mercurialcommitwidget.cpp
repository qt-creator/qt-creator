/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "mercurialcommitwidget.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/texteditorconstants.h>
#include <utils/completingtextedit.h>
#include <utils/qtcassert.h>

#include <QRegExp>
#include <QSyntaxHighlighter>
#include <QTextEdit>

//see the git submit widget for details of the syntax Highlighter

namespace Mercurial {
namespace Internal {

// Highlighter for Mercurial submit messages. Make the first line bold, indicates
// comments as such (retrieving the format from the text editor) and marks up
// keywords (words in front of a colon as in 'Task: <bla>').
class MercurialSubmitHighlighter : TextEditor::SyntaxHighlighter
{
public:
    explicit MercurialSubmitHighlighter(QTextEdit *parent);
    void highlightBlock(const QString &text);

private:
    enum State { None = -1, Header, Other };
    enum Format { Format_Comment };
    QRegExp m_keywordPattern;
};

MercurialSubmitHighlighter::MercurialSubmitHighlighter(QTextEdit *parent) :
        TextEditor::SyntaxHighlighter(parent),
        m_keywordPattern(QLatin1String("^\\w+:"))
{
    static const QVector<TextEditor::TextStyle> categories({TextEditor::C_COMMENT});

    setTextFormatCategories(categories);
    QTC_CHECK(m_keywordPattern.isValid());
}

void MercurialSubmitHighlighter::highlightBlock(const QString &text)
{
    // figure out current state
    State state = static_cast<State>(previousBlockState());
    if (text.startsWith(QLatin1String("HG:"))) {
        setFormat(0, text.size(), formatForCategory(Format_Comment));
        setCurrentBlockState(state);
        return;
    }

    if (state == None) {
        if (text.isEmpty()) {
            setCurrentBlockState(state);
            return;
        }
        state = Header;
    } else if (state == Header) {
        state = Other;
    }

    setCurrentBlockState(state);

    // Apply format.
    switch (state) {
    case None:
        break;
    case Header: {
        QTextCharFormat charFormat = format(0);
        charFormat.setFontWeight(QFont::Bold);
        setFormat(0, text.size(), charFormat);
        break;
    }
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


MercurialCommitWidget::MercurialCommitWidget() :
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
    const QString author = mercurialCommitPanelUi.authorLineEdit->text();
    const QString email = mercurialCommitPanelUi.emailLineEdit->text();
    if (author.isEmpty())
        return QString();

    QString user = author;
    if (!email.isEmpty()) {
        user += QLatin1String(" <");
        user += email;
        user += QLatin1Char('>');
    }
    return user;
}

QString MercurialCommitWidget::repoRoot()
{
    return mercurialCommitPanelUi.repositoryLabel->text();
}

QString MercurialCommitWidget::cleanupDescription(const QString &input) const
{
    const QRegularExpression commentLine(QLatin1String("^HG:[^\\n]*(\\n|$)"),
                                         QRegularExpression::MultilineOption);
    QString message = input;
    message.remove(commentLine);
    return message;
}

} // namespace Internal
} // namespace Mercurial
