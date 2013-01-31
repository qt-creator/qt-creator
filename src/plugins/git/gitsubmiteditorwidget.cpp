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

#include "gitsubmiteditorwidget.h"
#include "commitdata.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <utils/qtcassert.h>

#include <QLineEdit>
#include <QRegExpValidator>
#include <QSyntaxHighlighter>
#include <QTextEdit>

#include <QDebug>
#include <QDir>
#include <QRegExp>

namespace Git {
namespace Internal {

// Retrieve the comment char format from the text editor.
static QTextCharFormat commentFormat()
{
    const TextEditor::FontSettings settings = TextEditor::TextEditorSettings::instance()->fontSettings();
    return settings.toTextCharFormat(TextEditor::C_COMMENT);
}

// Highlighter for git submit messages. Make the first line bold, indicates
// comments as such (retrieving the format from the text editor) and marks up
// keywords (words in front of a colon as in 'Task: <bla>').

class GitSubmitHighlighter : QSyntaxHighlighter {
public:
    explicit GitSubmitHighlighter(QTextEdit *parent);
    void highlightBlock(const QString &text);

private:
    enum State { Header, Comment, Other };
    const QTextCharFormat m_commentFormat;
    QRegExp m_keywordPattern;
    const QChar m_hashChar;
};

GitSubmitHighlighter::GitSubmitHighlighter(QTextEdit * parent) :
    QSyntaxHighlighter(parent),
    m_commentFormat(commentFormat()),
    m_keywordPattern(QLatin1String("^\\w+:")),
    m_hashChar(QLatin1Char('#'))
{
    QTC_CHECK(m_keywordPattern.isValid());
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
    VcsBase::SubmitEditorWidget(parent),
    m_gitSubmitPanel(new QWidget),
    m_hasUnmerged(false)
{
    m_gitSubmitPanelUi.setupUi(m_gitSubmitPanel);
    insertTopWidget(m_gitSubmitPanel);
    new GitSubmitHighlighter(descriptionEdit());

    m_emailValidator = new QRegExpValidator(QRegExp(QLatin1String("[^@ ]+@[^@ ]+\\.[a-zA-Z]+")), this);

    connect(m_gitSubmitPanelUi.authorLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(authorInformationChanged()));
    connect(m_gitSubmitPanelUi.emailLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(authorInformationChanged()));
}

void GitSubmitEditorWidget::setPanelInfo(const GitSubmitEditorPanelInfo &info)
{
    m_gitSubmitPanelUi.repositoryLabel->setText(QDir::toNativeSeparators(info.repository));
    if (info.branch.contains(QLatin1String("(no branch)")))
        m_gitSubmitPanelUi.branchLabel->setText(QString::fromLatin1("<span style=\"color:red\">%1</span>")
                                                .arg(tr("Detached HEAD")));
    else
        m_gitSubmitPanelUi.branchLabel->setText(info.branch);
}

void GitSubmitEditorWidget::setHasUnmerged(bool e)
{
    m_hasUnmerged = e;
}

GitSubmitEditorPanelData GitSubmitEditorWidget::panelData() const
{
    GitSubmitEditorPanelData rc;
    rc.author = m_gitSubmitPanelUi.authorLineEdit->text();
    rc.email = m_gitSubmitPanelUi.emailLineEdit->text();
    rc.bypassHooks = m_gitSubmitPanelUi.bypassHooksCheckBox->isChecked();
    return rc;
}

void GitSubmitEditorWidget::setPanelData(const GitSubmitEditorPanelData &data)
{
    m_gitSubmitPanelUi.authorLineEdit->setText(data.author);
    m_gitSubmitPanelUi.emailLineEdit->setText(data.email);
    m_gitSubmitPanelUi.bypassHooksCheckBox->setChecked(data.bypassHooks);
    authorInformationChanged();
}

bool GitSubmitEditorWidget::canSubmit() const
{
    if (m_gitSubmitPanelUi.invalidAuthorLabel->isVisible()
        || m_gitSubmitPanelUi.invalidEmailLabel->isVisible()
        || m_hasUnmerged)
        return false;
    return SubmitEditorWidget::canSubmit();
}

QString GitSubmitEditorWidget::cleanupDescription(const QString &input) const
{
    // We need to manually purge out comment lines starting with
    // hash '#' since git does not do that when using -F.
    const QChar newLine = QLatin1Char('\n');
    const QChar hash = QLatin1Char('#');
    QString message = input;
    for (int pos = 0; pos < message.size(); ) {
        const int newLinePos = message.indexOf(newLine, pos);
        const int startOfNextLine = newLinePos == -1 ? message.size() : newLinePos + 1;
        if (message.at(pos) == hash)
            message.remove(pos, startOfNextLine - pos);
        else
            pos = startOfNextLine;
    }
    return message;

}

void GitSubmitEditorWidget::authorInformationChanged()
{
    bool bothEmpty = m_gitSubmitPanelUi.authorLineEdit->text().isEmpty() &&
            m_gitSubmitPanelUi.emailLineEdit->text().isEmpty();

    m_gitSubmitPanelUi.invalidAuthorLabel->
            setVisible(m_gitSubmitPanelUi.authorLineEdit->text().isEmpty() && !bothEmpty);
    m_gitSubmitPanelUi.invalidEmailLabel->
            setVisible(!emailIsValid() && !bothEmpty);

   updateSubmitAction();
}

bool GitSubmitEditorWidget::emailIsValid() const
{
    int pos = m_gitSubmitPanelUi.emailLineEdit->cursorPosition();
    QString text = m_gitSubmitPanelUi.emailLineEdit->text();
    return m_emailValidator->validate(text, pos) == QValidator::Acceptable;
}

} // namespace Internal
} // namespace Git
