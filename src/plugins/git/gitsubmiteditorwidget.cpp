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

#include "gitsubmiteditorwidget.h"
#include "commitdata.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>

#include <QtGui/QLineEdit>
#include <QtGui/QRegExpValidator>
#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QTextEdit>

#include <QtCore/QDebug>
#include <QtCore/QDir>
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

    m_emailValidator = new QRegExpValidator(QRegExp(QLatin1String("[^@ ]+@[^@ ]+\\.[a-zA-Z]+")), this);

    connect(m_gitSubmitPanelUi.authorLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(authorInformationChanged()));
    connect(m_gitSubmitPanelUi.emailLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(authorInformationChanged()));
}

void GitSubmitEditorWidget::setPanelInfo(const GitSubmitEditorPanelInfo &info)
{
    m_gitSubmitPanelUi.repositoryLabel->setText(QDir::toNativeSeparators(info.repository));
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
    authorInformationChanged();
}

bool GitSubmitEditorWidget::canSubmit() const
{
    QString message = cleanupDescription(descriptionText()).trimmed();

    if (m_gitSubmitPanelUi.invalidAuthorLabel->isVisible()
        || m_gitSubmitPanelUi.invalidEmailLabel->isVisible()
        || message.isEmpty())
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
        if (message.at(pos) == hash) {
            message.remove(pos, startOfNextLine - pos);
        } else {
            pos = startOfNextLine;
        }
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
