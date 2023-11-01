// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mercurialcommitwidget.h"

#include "mercurialtr.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/texteditorconstants.h>

#include <utils/completingtextedit.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextEdit>

//see the git submit widget for details of the syntax Highlighter

namespace Mercurial::Internal {

// Highlighter for Mercurial submit messages. Make the first line bold, indicates
// comments as such (retrieving the format from the text editor) and marks up
// keywords (words in front of a colon as in 'Task: <bla>').
class MercurialSubmitHighlighter : TextEditor::SyntaxHighlighter
{
public:
    explicit MercurialSubmitHighlighter(QTextEdit *parent);
    void highlightBlock(const QString &text) override;

private:
    enum State { None = -1, Header, Other };
    const QRegularExpression m_keywordPattern;
};

MercurialSubmitHighlighter::MercurialSubmitHighlighter(QTextEdit *parent) :
        TextEditor::SyntaxHighlighter(parent),
        m_keywordPattern(QLatin1String("^\\w+:"))
{
    QTC_CHECK(m_keywordPattern.isValid());
    setDefaultTextFormatCategories();
}

void MercurialSubmitHighlighter::highlightBlock(const QString &text)
{
    // figure out current state
    auto state = static_cast<State>(previousBlockState());
    if (text.startsWith(QLatin1String("HG:"))) {
        setFormat(0, text.size(), formatForCategory(TextEditor::C_COMMENT));
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
        const QRegularExpressionMatch match = m_keywordPattern.match(text);
        if (match.hasMatch() && match.capturedStart() == 0) {
            QTextCharFormat charFormat = format(0);
            charFormat.setFontItalic(true);
            setFormat(0, match.capturedLength(), charFormat);
        }
        break;
    }
}

class MercurialCommitPanel : public QWidget
{
public:
    MercurialCommitPanel()
    {
        m_repositoryLabel = new QLabel;
        m_branchLabel = new QLabel;

        m_authorLineEdit = new QLineEdit;
        m_emailLineEdit = new QLineEdit;

        using namespace Layouting;

        Column {
            Group {
                title(Tr::tr("General Information")),
                Form {
                    Tr::tr("Repository:"), m_repositoryLabel, br,
                    Tr::tr("Branch:"), m_branchLabel,
                }
            },
            Group {
                title(Tr::tr("Commit Information")),
                Row {
                    Form {
                        Tr::tr("Author:"), m_authorLineEdit, br,
                        Tr::tr("Email:"), m_emailLineEdit,
                    },
                }
            },
            noMargin
        }.attachTo(this);
    }

    QLabel *m_repositoryLabel;
    QLabel *m_branchLabel;
    QLineEdit *m_authorLineEdit;
    QLineEdit *m_emailLineEdit;
};

MercurialCommitWidget::MercurialCommitWidget()
    : mercurialCommitPanel(new MercurialCommitPanel)
{
    insertTopWidget(mercurialCommitPanel);
    new MercurialSubmitHighlighter(descriptionEdit());
}

void MercurialCommitWidget::setFields(const QString &repositoryRoot, const QString &branch,
                                      const QString &userName, const QString &email)
{
    mercurialCommitPanel->m_repositoryLabel->setText(repositoryRoot);
    mercurialCommitPanel->m_branchLabel->setText(branch);
    mercurialCommitPanel->m_authorLineEdit->setText(userName);
    mercurialCommitPanel->m_emailLineEdit->setText(email);
}

QString MercurialCommitWidget::committer() const
{
    const QString author = mercurialCommitPanel->m_authorLineEdit->text();
    const QString email = mercurialCommitPanel->m_emailLineEdit->text();
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

QString MercurialCommitWidget::repoRoot() const
{
    return mercurialCommitPanel->m_repositoryLabel->text();
}

QString MercurialCommitWidget::cleanupDescription(const QString &input) const
{
    const QRegularExpression commentLine(QLatin1String("^HG:[^\\n]*(\\n|$)"),
                                         QRegularExpression::MultilineOption);
    QString message = input;
    message.remove(commentLine);
    return message;
}

} // Mercurial::Internal
