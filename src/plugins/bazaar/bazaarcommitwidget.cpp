// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bazaarcommitwidget.h"

#include "bazaartr.h"
#include "branchinfo.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>

#include <utils/completingtextedit.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QDebug>
#include <QLineEdit>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextEdit>

//see the git submit widget for details of the syntax Highlighter

//TODO Check to see when the Highlighter has been moved to a base class and use that instead

namespace Bazaar::Internal {

// Retrieve the comment char format from the text editor.
static QTextCharFormat commentFormat()
{
    return TextEditor::TextEditorSettings::fontSettings().toTextCharFormat(TextEditor::C_COMMENT);
}

class BazaarCommitPanel : public QWidget
{
public:
    BazaarCommitPanel()
    {
        branchLineEdit = new QLineEdit;
        branchLineEdit->setReadOnly(true);

        isLocalCheckBox = new QCheckBox(Tr::tr("Local commit"));
        isLocalCheckBox->setToolTip(Tr::tr("Performs a local commit in a bound branch.\n"
                                           "Local commits are not pushed to the master "
                                           "branch until a normal commit is performed."));

        authorLineEdit = new QLineEdit;
        emailLineEdit = new QLineEdit;
        fixedBugsLineEdit = new QLineEdit;

        using namespace Layouting;
        Column {
            Group {
                title(Tr::tr("General Information")),
                Form {
                    Tr::tr("Branch:"), branchLineEdit, br,
                    empty, isLocalCheckBox
                }
            },
            Group {
                title(Tr::tr("Commit Information")),
                Form {
                    Tr::tr("Author:"), authorLineEdit, br,
                    Tr::tr("Email:"), emailLineEdit, br,
                    Tr::tr("Fixed bugs:"), fixedBugsLineEdit
                }
            },
            noMargin
        }.attachTo(this);
    }

    QLineEdit *branchLineEdit;
    QCheckBox *isLocalCheckBox;
    QLineEdit *authorLineEdit;
    QLineEdit *emailLineEdit;
    QLineEdit *fixedBugsLineEdit;
};

// Highlighter for Bazaar submit messages. Make the first line bold, indicates
// comments as such (retrieving the format from the text editor) and marks up
// keywords (words in front of a colon as in 'Task: <bla>').
class BazaarSubmitHighlighter : QSyntaxHighlighter
{
public:
    explicit BazaarSubmitHighlighter(QTextEdit *parent);
    void highlightBlock(const QString &text) override;

private:
    enum State { Header, Comment, Other };
    const QTextCharFormat m_commentFormat;
    QRegularExpression m_keywordPattern;
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
    case Other: {
        // Format key words ("Task:") italic
        const QRegularExpressionMatch match = m_keywordPattern.match(text);
        if (match.hasMatch()) {
            QTextCharFormat charFormat = format(0);
            charFormat.setFontItalic(true);
            setFormat(0, match.capturedLength(), charFormat);
        }
        break;
    }
    }
}


BazaarCommitWidget::BazaarCommitWidget()
    : m_bazaarCommitPanel(new BazaarCommitPanel)
{
    insertTopWidget(m_bazaarCommitPanel);
    new BazaarSubmitHighlighter(descriptionEdit());
}

void BazaarCommitWidget::setFields(const BranchInfo &branch,
                                   const QString &userName, const QString &email)
{
    m_bazaarCommitPanel->branchLineEdit->setText(branch.branchLocation);
    m_bazaarCommitPanel->isLocalCheckBox->setVisible(branch.isBoundToBranch);
    m_bazaarCommitPanel->authorLineEdit->setText(userName);
    m_bazaarCommitPanel->emailLineEdit->setText(email);
}

QString BazaarCommitWidget::committer() const
{
    const QString author = m_bazaarCommitPanel->authorLineEdit->text();
    const QString email = m_bazaarCommitPanel->emailLineEdit->text();
    if (author.isEmpty())
        return {};

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
    return m_bazaarCommitPanel->fixedBugsLineEdit->text().split(QRegularExpression("\\s+"));
}

bool BazaarCommitWidget::isLocalOptionEnabled() const
{
    return m_bazaarCommitPanel->isLocalCheckBox->isChecked();
}

} // Bazaar::Internal
