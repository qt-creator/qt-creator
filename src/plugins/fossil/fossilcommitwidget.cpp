// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fossilcommitwidget.h"

#include "branchinfo.h"
#include "fossiltr.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>

#include <utils/completingtextedit.h>
#include <utils/filepath.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QDir>
#include <QLineEdit>
#include <QRegularExpression>
#include <QSyntaxHighlighter>

using namespace Utils;

namespace Fossil {
namespace Internal {

// Retrieve the comment char format from the text editor.
static QTextCharFormat commentFormat()
{
    const TextEditor::FontSettings &settings = TextEditor::TextEditorSettings::instance()->fontSettings();
    return settings.toTextCharFormat(TextEditor::C_COMMENT);
}

// Highlighter for Fossil submit messages.
// Marks up [ticket-id] fields in the message.
class FossilSubmitHighlighter : QSyntaxHighlighter
{
public:
    explicit FossilSubmitHighlighter(CompletingTextEdit *parent);
    void highlightBlock(const QString &text) final;

private:
    const QTextCharFormat m_commentFormat;
    const QRegularExpression m_keywordPattern;
};

FossilSubmitHighlighter::FossilSubmitHighlighter(CompletingTextEdit *parent) : QSyntaxHighlighter(parent),
    m_commentFormat(commentFormat()),
    m_keywordPattern("\\[([0-9a-f]{5,40})\\]")
{
    QTC_CHECK(m_keywordPattern.isValid());
}

void FossilSubmitHighlighter::highlightBlock(const QString &text)
{
    // Fossil commit message allows listing of [ticket-id],
    // where ticket-id is a partial SHA1.
    // Match the ticket-ids and highlight them for convenience.

    // Format keywords
    QRegularExpressionMatchIterator i = m_keywordPattern.globalMatch(text);
    while (i.hasNext()) {
        const QRegularExpressionMatch keywordMatch = i.next();
        QTextCharFormat charFormat = format(0);
        charFormat.setFontItalic(true);
        setFormat(keywordMatch.capturedStart(0), keywordMatch.capturedLength(0), charFormat);
    }
}


FossilCommitWidget::FossilCommitWidget() : m_commitPanel(new QWidget)
{
    m_localRootLineEdit = new QLineEdit;
    m_localRootLineEdit->setFocusPolicy(Qt::NoFocus);
    m_localRootLineEdit->setReadOnly(true);

    m_currentBranchLineEdit = new QLineEdit;
    m_currentBranchLineEdit->setFocusPolicy(Qt::NoFocus);
    m_currentBranchLineEdit->setReadOnly(true);

    m_currentTagsLineEdit = new QLineEdit;
    m_currentTagsLineEdit->setFocusPolicy(Qt::NoFocus);
    m_currentTagsLineEdit->setReadOnly(true);

    m_branchLineEdit = new QLineEdit;

    m_invalidBranchLabel = new InfoLabel;
    m_invalidBranchLabel->setMinimumSize(QSize(50, 20));
    m_invalidBranchLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    m_invalidBranchLabel->setType(InfoLabel::Error);

    m_isPrivateCheckBox = new QCheckBox(Tr::tr("Private"));
    m_isPrivateCheckBox->setToolTip("<html>" + Tr::tr("Create a private check-in that is never synced. "
                                       "Children of private check-ins are automatically private. "
                                       "Private check-ins are not pushed to the remote repository by default."));

    m_tagsLineEdit = new QLineEdit;
    m_tagsLineEdit->setToolTip(Tr::tr("Tag names to apply; comma-separated."));

    m_authorLineEdit = new QLineEdit;

    using namespace Layouting;

    Column {
        Group {
            title(Tr::tr("Current Information")),
            Form {
                Tr::tr("Local root:"), m_localRootLineEdit,
                Tr::tr("Branch:"), m_currentBranchLineEdit,
                Tr::tr("Tags:"), m_currentTagsLineEdit
            }
        },
        Group {
            title(Tr::tr("Commit Information")),
            Grid {
                Tr::tr("New branch:"), m_branchLineEdit,  m_invalidBranchLabel, m_isPrivateCheckBox, br,
                Tr::tr("Tags:"), m_tagsLineEdit, br,
                Tr::tr("Author:"),  m_authorLineEdit, st,
            }
        },
        noMargin
    }.attachTo(m_commitPanel);

    insertTopWidget(m_commitPanel);
    new FossilSubmitHighlighter(descriptionEdit());
    m_branchValidator = new QRegularExpressionValidator(QRegularExpression("[^\\n]*"), this);

    connect(m_branchLineEdit, &QLineEdit::textChanged,
            this, &FossilCommitWidget::branchChanged);
}

void FossilCommitWidget::setFields(const FilePath &repoPath, const BranchInfo &branch,
                                   const QStringList &tags, const QString &userName)
{
    m_localRootLineEdit->setText(repoPath.toUserOutput());
    m_currentBranchLineEdit->setText(branch.name);
    m_currentTagsLineEdit->setText(tags.join(", "));
    m_authorLineEdit->setText(userName);

    branchChanged();
}

QString FossilCommitWidget::newBranch() const
{
    return m_branchLineEdit->text().trimmed();
}

QStringList FossilCommitWidget::tags() const
{
    QString tagsText = m_tagsLineEdit->text().trimmed();
    if (tagsText.isEmpty())
        return {};

    return tagsText.replace(',', ' ').split(' ', Qt::SkipEmptyParts);
}

QString FossilCommitWidget::committer() const
{
    return m_authorLineEdit->text();
}

bool FossilCommitWidget::isPrivateOptionEnabled() const
{
    return m_isPrivateCheckBox->isChecked();
}

bool FossilCommitWidget::canSubmit(QString *whyNot) const
{
    QString message = cleanupDescription(descriptionText()).trimmed();

    if (m_invalidBranchLabel->isVisible() || message.isEmpty()) {
        if (whyNot)
            *whyNot = Tr::tr("Message check failed.");
        return false;
    }

    return VcsBase::SubmitEditorWidget::canSubmit();
}

void FossilCommitWidget::branchChanged()
{
    m_invalidBranchLabel->setVisible(!isValidBranch());

    updateSubmitAction();
}

bool FossilCommitWidget::isValidBranch() const
{
    int pos = m_branchLineEdit->cursorPosition();
    QString text = m_branchLineEdit->text();
    return m_branchValidator->validate(text, pos) == QValidator::Acceptable;
}

} // namespace Internal
} // namespace Fossil
