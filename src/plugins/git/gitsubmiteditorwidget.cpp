// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitsubmiteditorwidget.h"

#include "commitdata.h"
#include "gitconstants.h"
#include "githighlighters.h"
#include "gittr.h"
#include "logchangedialog.h"

#include <coreplugin/coreconstants.h>

#include <utils/completingtextedit.h>
#include <utils/filepath.h>
#include <utils/layoutbuilder.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QRegularExpressionValidator>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace Utils;

namespace Git::Internal {

class GitSubmitPanel : public QWidget
{
public:
    GitSubmitPanel()
    {
        repositoryLabel = new QLabel(Tr::tr("repository"));
        branchLabel = new QLabel(Tr::tr("branch")); // FIXME: Isn't this overwritten soon?
        showHeadLabel = new QLabel("<a href=\"head\">" + Tr::tr("Show HEAD") + "</a>");

        authorLineEdit = new QLineEdit;
        authorLineEdit->setObjectName("authorLineEdit");
        authorLineEdit->setMinimumSize(QSize(200, 0));

        invalidAuthorLabel = new QLabel;
        invalidAuthorLabel->setObjectName("invalidAuthorLabel");
        invalidAuthorLabel->setMinimumSize(QSize(20, 20));

        emailLineEdit = new QLineEdit;
        emailLineEdit->setObjectName("emailLineEdit");
        emailLineEdit->setMinimumSize(QSize(200, 0));

        invalidEmailLabel = new QLabel;
        invalidEmailLabel->setObjectName("invalidEmailLabel");
        invalidEmailLabel->setMinimumSize(QSize(20, 20));

        bypassHooksCheckBox = new QCheckBox(Tr::tr("By&pass hooks"));

        signOffCheckBox = new QCheckBox(Tr::tr("Sign off"));

        using namespace Layouting;

        editGroup = new QGroupBox(Tr::tr("Commit Information"));
        Grid {
            Tr::tr("Author:"), authorLineEdit, invalidAuthorLabel, st, br,
            Tr::tr("Email:"), emailLineEdit, invalidEmailLabel, br,
            empty, Row { bypassHooksCheckBox, signOffCheckBox, st }
        }.attachTo(editGroup);

        Column {
            Group {
                title(Tr::tr("General Information")),
                Form {
                    Tr::tr("Repository:"), repositoryLabel, br,
                    Tr::tr("Branch:"), branchLabel, br,
                    Span(2, showHeadLabel)
                }
            },
            editGroup,
            noMargin,
        }.attachTo(this);
    }

    QLabel *repositoryLabel;
    QLabel *branchLabel;
    QLabel *showHeadLabel;
    QGroupBox *editGroup;
    QLineEdit *authorLineEdit;
    QLabel *invalidAuthorLabel;
    QLineEdit *emailLineEdit;
    QLabel *invalidEmailLabel;
    QCheckBox *bypassHooksCheckBox;
    QCheckBox *signOffCheckBox;
};

// ------------------
GitSubmitEditorWidget::GitSubmitEditorWidget() :
    m_gitSubmitPanel(new GitSubmitPanel)
{
    m_highlighter = new GitSubmitHighlighter(QChar(), descriptionEdit());

    m_emailValidator = new QRegularExpressionValidator(QRegularExpression("[^@ ]+@[^@ ]+\\.[a-zA-Z]+"), this);
    const QPixmap error = Utils::Icons::CRITICAL.pixmap();
    m_gitSubmitPanel->invalidAuthorLabel->setPixmap(error);
    m_gitSubmitPanel->invalidEmailLabel->setToolTip(Tr::tr("Provide a valid email to commit."));
    m_gitSubmitPanel->invalidEmailLabel->setPixmap(error);

    connect(m_gitSubmitPanel->authorLineEdit, &QLineEdit::textChanged,
            this, &GitSubmitEditorWidget::authorInformationChanged);
    connect(m_gitSubmitPanel->emailLineEdit, &QLineEdit::textChanged,
            this, &GitSubmitEditorWidget::authorInformationChanged);
    connect(m_gitSubmitPanel->showHeadLabel, &QLabel::linkActivated,
            this, [this] { emit showRequested("HEAD"); });
}

void GitSubmitEditorWidget::setPanelInfo(const GitSubmitEditorPanelInfo &info)
{
    m_gitSubmitPanel->repositoryLabel->setText(info.repository.toUserOutput());
    if (info.branch.contains("(no branch)")) {
        const QString errorColor =
                Utils::creatorTheme()->color(Utils::Theme::TextColorError).name();
        m_gitSubmitPanel->branchLabel->setText(QString::fromLatin1("<span style=\"color:%1\">%2</span>")
                                                .arg(errorColor, Tr::tr("Detached HEAD")));
    } else {
        m_gitSubmitPanel->branchLabel->setText(info.branch);
    }
}

QString GitSubmitEditorWidget::amendSHA1() const
{
    return m_logChangeWidget ? m_logChangeWidget->commit() : QString();
}

void GitSubmitEditorWidget::setHasUnmerged(bool e)
{
    m_hasUnmerged = e;
}

void GitSubmitEditorWidget::initialize(const FilePath &repository, const CommitData &data)
{
    if (m_isInitialized)
        return;
    m_isInitialized = true;
    if (data.commitType != AmendCommit)
        m_gitSubmitPanel->showHeadLabel->hide();
    if (data.commitType == FixupCommit) {
        auto logChangeGroupBox = new QGroupBox(Tr::tr("Select Change"));
        auto logChangeLayout = new QVBoxLayout;
        logChangeGroupBox->setLayout(logChangeLayout);
        m_logChangeWidget = new LogChangeWidget;
        m_logChangeWidget->init(repository);
        connect(m_logChangeWidget, &LogChangeWidget::commitActivated, this, &GitSubmitEditorWidget::showRequested);
        logChangeLayout->addWidget(m_logChangeWidget);
        insertLeftWidget(logChangeGroupBox);
        m_gitSubmitPanel->editGroup->hide();
        hideDescription();
    } else {
        m_highlighter->setCommentChar(data.commentChar);
        if (data.commentChar != Constants::DEFAULT_COMMENT_CHAR)
            verifyDescription();
    }
    insertTopWidget(m_gitSubmitPanel);
    setPanelData(data.panelData);
    setPanelInfo(data.panelInfo);

    if (data.enablePush) {
        auto menu = new QMenu(this);
        connect(menu->addAction(Tr::tr("&Commit only")), &QAction::triggered,
                this, &GitSubmitEditorWidget::commitOnlySlot);
        connect(menu->addAction(Tr::tr("Commit and &Push")), &QAction::triggered,
                this, &GitSubmitEditorWidget::commitAndPushSlot);
        connect(menu->addAction(Tr::tr("Commit and Push to &Gerrit")), &QAction::triggered,
                this, &GitSubmitEditorWidget::commitAndPushToGerritSlot);
        addSubmitButtonMenu(menu);
    }
}

void GitSubmitEditorWidget::refreshLog(const FilePath &repository)
{
    if (m_logChangeWidget)
        m_logChangeWidget->init(repository);
}

GitSubmitEditorPanelData GitSubmitEditorWidget::panelData() const
{
    GitSubmitEditorPanelData rc;
    const QString author = m_gitSubmitPanel->authorLineEdit->text();
    const QString email = m_gitSubmitPanel->emailLineEdit->text();
    if (author != m_originalAuthor || email != m_originalEmail) {
        rc.author = author;
        rc.email = email;
    }
    rc.bypassHooks = m_gitSubmitPanel->bypassHooksCheckBox->isChecked();
    rc.pushAction = m_pushAction;
    rc.signOff = m_gitSubmitPanel->signOffCheckBox->isChecked();
    return rc;
}

void GitSubmitEditorWidget::setPanelData(const GitSubmitEditorPanelData &data)
{
    m_originalAuthor = data.author;
    m_originalEmail = data.email;
    m_gitSubmitPanel->authorLineEdit->setText(data.author);
    m_gitSubmitPanel->emailLineEdit->setText(data.email);
    m_gitSubmitPanel->bypassHooksCheckBox->setChecked(data.bypassHooks);
    m_gitSubmitPanel->signOffCheckBox->setChecked(data.signOff);
    authorInformationChanged();
}

bool GitSubmitEditorWidget::canSubmit(QString *whyNot) const
{
    if (m_gitSubmitPanel->invalidAuthorLabel->isVisible()) {
        if (whyNot)
            *whyNot = Tr::tr("Invalid author");
        return false;
    }
    if (m_gitSubmitPanel->invalidEmailLabel->isVisible()) {
        if (whyNot)
            *whyNot = Tr::tr("Invalid email");
        return false;
    }
    if (m_hasUnmerged) {
        if (whyNot)
            *whyNot = Tr::tr("Unresolved merge conflicts");
        return false;
    }
    return SubmitEditorWidget::canSubmit(whyNot);
}

QString GitSubmitEditorWidget::cleanupDescription(const QString &input) const
{
    // We need to manually purge out comment lines starting with
    // the comment char (default hash '#') since git does not do that when using -F.
    const QChar newLine = '\n';
    const QChar commentChar = m_highlighter->commentChar();
    QString message = input;
    for (int pos = 0; pos < message.size(); ) {
        const int newLinePos = message.indexOf(newLine, pos);
        const int startOfNextLine = newLinePos == -1 ? message.size() : newLinePos + 1;
        if (message.at(pos) == commentChar)
            message.remove(pos, startOfNextLine - pos);
        else
            pos = startOfNextLine;
    }
    return message;

}

QString GitSubmitEditorWidget::commitName() const
{
    if (m_pushAction == NormalPush)
        return Tr::tr("&Commit and Push");
    else if (m_pushAction == PushToGerrit)
        return Tr::tr("&Commit and Push to Gerrit");

    return Tr::tr("&Commit");
}

void GitSubmitEditorWidget::authorInformationChanged()
{
    bool bothEmpty = m_gitSubmitPanel->authorLineEdit->text().isEmpty() &&
            m_gitSubmitPanel->emailLineEdit->text().isEmpty();

    m_gitSubmitPanel->invalidAuthorLabel->
            setVisible(m_gitSubmitPanel->authorLineEdit->text().isEmpty() && !bothEmpty);
    m_gitSubmitPanel->invalidEmailLabel->
            setVisible(!emailIsValid() && !bothEmpty);

    updateSubmitAction();
}

void GitSubmitEditorWidget::commitOnlySlot()
{
    m_pushAction = NoPush;
    updateSubmitAction();
}

void GitSubmitEditorWidget::commitAndPushSlot()
{
    m_pushAction = NormalPush;
    updateSubmitAction();
}

void GitSubmitEditorWidget::commitAndPushToGerritSlot()
{
    m_pushAction = PushToGerrit;
    updateSubmitAction();
}

bool GitSubmitEditorWidget::emailIsValid() const
{
    int pos = m_gitSubmitPanel->emailLineEdit->cursorPosition();
    QString text = m_gitSubmitPanel->emailLineEdit->text();
    return m_emailValidator->validate(text, pos) == QValidator::Acceptable;
}

} // Git::Internal
