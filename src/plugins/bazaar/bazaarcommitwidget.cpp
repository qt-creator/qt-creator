/**************************************************************************
**
** Copyright (c) 2013 Hugues Delorme
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
#include "bazaarcommitwidget.h"
#include "branchinfo.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <utils/qtcassert.h>

#include <QSyntaxHighlighter>
#include <QTextEdit>

#include <QDebug>
#include <QRegExp>

//see the git submit widget for details of the syntax Highlighter

//TODO Check to see when the Highlighter has been moved to a base class and use that instead

namespace Bazaar {
namespace Internal {

// Retrieve the comment char format from the text editor.
static QTextCharFormat commentFormat()
{
    const TextEditor::FontSettings settings = TextEditor::TextEditorSettings::instance()->fontSettings();
    return settings.toTextCharFormat(TextEditor::C_COMMENT);
}

// Highlighter for Bazaar submit messages. Make the first line bold, indicates
// comments as such (retrieving the format from the text editor) and marks up
// keywords (words in front of a colon as in 'Task: <bla>').
class BazaarSubmitHighlighter : QSyntaxHighlighter
{
public:
    explicit BazaarSubmitHighlighter(QTextEdit *parent);
    void highlightBlock(const QString &text);

private:
    enum State { Header, Comment, Other };
    const QTextCharFormat m_commentFormat;
    QRegExp m_keywordPattern;
    const QChar m_hashChar;
};

BazaarSubmitHighlighter::BazaarSubmitHighlighter(QTextEdit * parent) :
    QSyntaxHighlighter(parent),
    m_commentFormat(commentFormat()),
    m_keywordPattern(QLatin1String("^\\w+:")),
    m_hashChar(QLatin1Char('#'))
{
    QTC_CHECK(m_keywordPattern.isValid());
}

void BazaarSubmitHighlighter::highlightBlock(const QString &text)
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


BazaarCommitWidget::BazaarCommitWidget(QWidget *parent) :
    VcsBase::SubmitEditorWidget(parent),
    m_bazaarCommitPanel(new QWidget)
{
    m_bazaarCommitPanelUi.setupUi(m_bazaarCommitPanel);
    insertTopWidget(m_bazaarCommitPanel);
    new BazaarSubmitHighlighter(descriptionEdit());
}

void BazaarCommitWidget::setFields(const BranchInfo &branch,
                                   const QString &userName, const QString &email)
{
    m_bazaarCommitPanelUi.branchLineEdit->setText(branch.branchLocation);
    m_bazaarCommitPanelUi.isLocalCheckBox->setVisible(branch.isBoundToBranch);
    m_bazaarCommitPanelUi.authorLineEdit->setText(userName);
    m_bazaarCommitPanelUi.emailLineEdit->setText(email);
}

QString BazaarCommitWidget::committer() const
{
    const QString author = m_bazaarCommitPanelUi.authorLineEdit->text();
    const QString email = m_bazaarCommitPanelUi.emailLineEdit->text();
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

QStringList BazaarCommitWidget::fixedBugs() const
{
    return m_bazaarCommitPanelUi.fixedBugsLineEdit->text().split(QRegExp(QLatin1String("\\s+")));
}

bool BazaarCommitWidget::isLocalOptionEnabled() const
{
    return m_bazaarCommitPanelUi.isLocalCheckBox->isChecked();
}

} // namespace Internal
} // namespace Bazaar
