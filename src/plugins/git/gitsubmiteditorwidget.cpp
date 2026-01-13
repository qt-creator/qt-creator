// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitsubmiteditorwidget.h"

#include "commitdata.h"
#include "gitconstants.h"
#include "githighlighters.h"
#include "gittr.h"
#include "logchangedialog.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/fileutils.h>

#include <utils/completingtextedit.h>
#include <utils/filepath.h>
#include <utils/layoutbuilder.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <vcsbase/submitfilemodel.h>

#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
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
        repositoryLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
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
            Tr::tr("Author:"), authorLineEdit, invalidAuthorLabel, br,
            Tr::tr("Email:"), emailLineEdit, invalidEmailLabel, br,
            empty, Row { bypassHooksCheckBox, signOffCheckBox, st }
        }.attachTo(editGroup);

        Row {
            Group {
                title(Tr::tr("General Information")),
                Form {
                    Tr::tr("Repository:"), repositoryLabel, br,
                    Tr::tr("Branch:"), branchLabel, br,
                    empty, showHeadLabel
                }
            },
            editGroup,
            noMargin,
        }.attachTo(this);
    }

    QLabel *repositoryLabel = nullptr;
    QLabel *branchLabel = nullptr;
    QLabel *showHeadLabel = nullptr;
    QGroupBox *editGroup = nullptr;
    QLineEdit *authorLineEdit = nullptr;
    QLabel *invalidAuthorLabel = nullptr;
    QLineEdit *emailLineEdit = nullptr;
    QLabel *invalidEmailLabel = nullptr;
    QCheckBox *bypassHooksCheckBox = nullptr;
    QCheckBox *signOffCheckBox = nullptr;
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
    connect(m_gitSubmitPanel->branchLabel, &QLabel::linkActivated,
            this, [this] { emit logRequested(m_range); });
}

void GitSubmitEditorWidget::setPanelInfo(const GitSubmitEditorPanelInfo &info)
{
    m_gitSubmitPanel->repositoryLabel->setText(info.repository.toUserOutput());
    QString label;
    if (info.branch.contains("(no branch)")) {
        const QString errorColor = Utils::creatorColor(Utils::Theme::TextColorError).name();
        m_range = {"HEAD", "--not", "--remotes"};
        label = QString("<a style=\"color: %1;\" href=\"branch\">%2</a>")
                    .arg(errorColor, Tr::tr("Detached HEAD"));
    } else {
        m_range = {info.branch.split(' ').first()}; // split removes "[ahead 3]"
        label = "<a href=\"branch\">" + info.branch + "</a> ";
    }
    m_gitSubmitPanel->branchLabel->setText(label);
}

QString GitSubmitEditorWidget::amendHash() const
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
        m_editMessageCheckBox = new QCheckBox(Tr::tr("Edit commit message"));
        m_editMessageCheckBox->setToolTip(Tr::tr("Opens an editor to edit the final commit message."));
        m_logChangeWidget = new LogChangeWidget;
        m_logChangeWidget->init(repository);
        connect(m_logChangeWidget, &LogChangeWidget::commitActivated, this, &GitSubmitEditorWidget::showRequested);
        logChangeLayout->addWidget(m_editMessageCheckBox);
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
    if (m_editMessageCheckBox)
        rc.editMessage = m_editMessageCheckBox->isChecked();
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

void GitSubmitEditorWidget::addFileContextMenuActions(QMenu *menu, const QModelIndex &index)
{
    using namespace Core;

    // Do not add context menu actions when multiple files are selected
    if (selectedRows().size() > 1)
        return;

    const VcsBase::SubmitFileModel *model = fileModel();
    const FilePath filePath = FilePath::fromString(model->file(index.row()));
    const FilePath fullFilePath = model->repositoryRoot().resolvePath(filePath);
    const FileStates state = static_cast<FileStates>(model->extraData(index.row()).toInt());

    menu->addSeparator();
    const auto addAction =
        [this,
         menu,
         filePath](const QString &title, IVersionControl::FileAction action, const QString &revertPrompt = {}) {
            QAction *act = menu->addAction(title.arg(filePath.toUserOutput()));
            connect(act, &QAction::triggered, this, [=, this] {
                if (!revertPrompt.isEmpty()) {
                    const int result = QMessageBox::question(
                        this,
                        Tr::tr("Confirm File Changes"),
                        revertPrompt.arg(filePath.toUserOutput()),
                        QMessageBox::Yes | QMessageBox::No);
                    if (result != QMessageBox::Yes)
                        return;
                }
                emit fileActionRequested(filePath, action);
            });
        };
    addAction(Tr::tr("Open \"%1\""), IVersionControl::FileOpenEditor);
    menu->addSeparator();
    addAction(Tr::tr("Copy \"%1\""), IVersionControl::FileCopyClipboard);
    Core::EditorManager::addContextMenuActions(menu, fullFilePath);
    menu->addSeparator();
    if (state & RenamedFile) {
        addAction(Tr::tr("Revert Renaming \"%1\""), IVersionControl::FileRevertRenaming);
    }
    if (state & (UnmergedFile | UnmergedThem | UnmergedUs)) {
        addAction(Tr::tr("Run Merge Tool for \"%1\""), IVersionControl::FileMergeTool);
        addAction(Tr::tr("Diff Incoming Changes for \"%1\""), IVersionControl::FileMergeDiffIncoming);

        if (state & DeletedFile) {
            addAction(Tr::tr("Resolve by Recovering \"%1\""), IVersionControl::FileMergeRecover);
            addAction(Tr::tr("Resolve by Removing \"%1\"..."), IVersionControl::FileMergeRemove,
                      Tr::tr("<p>Permanently remove file \"%1\"?</p>"
                             "<p>Note: The changes will be discarded.</p>"));
        } else {
            addAction(Tr::tr("Mark Conflicts Resolved for \"%1\""), IVersionControl::FileMergeResolved);
            addAction(Tr::tr("Resolve Conflicts in \"%1\" with Ours..."), IVersionControl::FileMergeOurs,
                      Tr::tr("<p>Resolve all conflicts to the file \"%1\" with <b>our</b> version?</p>"
                             "<p>Note: The other changes will be discarded.</p>"));
            addAction(Tr::tr("Resolve Conflicts in \"%1\" with Theirs..."), IVersionControl::FileMergeTheirs,
                      Tr::tr("<p>Resolve all conflicts to the file \"%1\" with <b>their</b> version?</p>"
                             "<p>Note: Our changes will be discarded.</p>"));
        }
    } else if (state & DeletedFile) {
        addAction(Tr::tr("Recover \"%1\""), IVersionControl::FileRevertDeletion);
    } else if (state & AddedFile) {
        if (state & StagedFile) {
            addAction(Tr::tr("Unstage \"%1\""), IVersionControl::FileUnstageAdded);
        } else {
            addAction(Tr::tr("Stage \"%1\""), IVersionControl::FileStage);
            addAction(Tr::tr("Mark Untracked \"%1\""), IVersionControl::FileUnstage);
            addAction(Tr::tr("Remove \"%1\"..."), IVersionControl::FileRemove,
                      Tr::tr("<p>Permanently remove the file \"%1\"?</p>"
                             "<p>Note: The deletion cannot be undone.</p>"));
        }
    } else if (state == (StagedFile | ModifiedFile)) {
        addAction(Tr::tr("Unstage \"%1\""), IVersionControl::FileUnstage);
        menu->addSeparator();
        addAction(
            Tr::tr("Revert All Changes to \"%1\"..."),
            IVersionControl::FileRevertAll,
            Tr::tr("<p>Undo <b>all</b> changes to the file \"%1\"?</p>"
                   "<p>Note: These changes will be lost.</p>"));
    } else if (state == ModifiedFile) {
        addAction(Tr::tr("Stage \"%1\""), IVersionControl::FileStage);
        menu->addSeparator();
        addAction(
            Tr::tr("Revert Unstaged Changes to \"%1\"..."),
            IVersionControl::FileRevertUnstaged,
            Tr::tr("<p>Undo unstaged changes to the file \"%1\"?</p>"
                   "<p>Note: These changes will be lost.</p>"));
    } else if (state == UntrackedFile) {
        addAction(Tr::tr("Add \"%1\""), IVersionControl::FileAdd);
        addAction(Tr::tr("Stage \"%1\""), IVersionControl::FileStage);
        menu->addSeparator();
        addAction(Tr::tr("Remove \"%1\"..."), IVersionControl::FileRemove,
                  Tr::tr("<p>Permanently remove the file \"%1\"?</p>"
                         "<p>Note: The deletion cannot be undone.</p>"));
        menu->addSeparator();
        const char message[] = "Add to gitignore \"%1\"";
        addAction(Tr::tr(message).arg("/" + filePath.path()), IVersionControl::FileAddGitignore);
        const std::optional<Utils::FilePath> path = filePath.tailRemoved(filePath.fileName());
        if (!path.has_value())
            return;

        const QString baseName = filePath.completeBaseName();
        const QString suffix = filePath.suffix();
        if (baseName.isEmpty() || suffix.isEmpty())
            return;

        const Utils::FilePath suffixMask = path->stringAppended("*." + suffix);
        QAction *act0 = menu->addAction(Tr::tr(message).arg("/" + suffixMask.path()));
        connect(act0, &QAction::triggered, this, [suffixMask, this] {
            emit fileActionRequested(suffixMask, IVersionControl::FileAddGitignore);
        });
        const Utils::FilePath nameMask = path->stringAppended(baseName + ".*");
        QAction *act1 = menu->addAction(Tr::tr(message).arg("/" + nameMask.path()));
        connect(act1, &QAction::triggered, this, [nameMask, this] {
            emit fileActionRequested(nameMask, IVersionControl::FileAddGitignore);
        });
    }
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
