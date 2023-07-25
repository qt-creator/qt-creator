// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "changeselectiondialog.h"

#include "logchangedialog.h"
#include "gitclient.h"
#include "gittr.h"

#include <coreplugin/vcsmanager.h>

#include <utils/completinglineedit.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/theme/theme.h>

#include <QCompleter>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStringListModel>
#include <QTimer>

using namespace Utils;
using namespace VcsBase;

namespace Git::Internal {

ChangeSelectionDialog::ChangeSelectionDialog(const FilePath &workingDirectory, Id id,
                                             QWidget *parent) :
    QDialog(parent)
{
    m_gitExecutable = gitClient().vcsBinary();
    m_gitEnvironment = gitClient().processEnvironment();

    resize(550, 350);
    setWindowTitle(Tr::tr("Select a Git Commit"));
    setObjectName("Git.ChangeSelectionDialog");

    m_workingDirectoryChooser = new PathChooser(this);

    m_changeNumberEdit = new CompletingLineEdit(this);
    m_changeNumberEdit->setObjectName("changeNumberEdit");
    m_changeNumberEdit->setText(Tr::tr("HEAD"));
    m_changeNumberEdit->setFocus();
    m_changeNumberEdit->selectAll();

    m_detailsText = new QPlainTextEdit(this);
    m_detailsText->setObjectName("detailsText");
    m_detailsText->setUndoRedoEnabled(false);
    m_detailsText->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_detailsText->setReadOnly(true);

    auto selectFromHistoryButton = new QPushButton(Tr::tr("Browse &History..."));
    auto closeButton = new QPushButton(Tr::tr("&Close"));
    auto archiveButton = new QPushButton(Tr::tr("&Archive..."));

    m_checkoutButton = new QPushButton(Tr::tr("Check&out"));
    m_revertButton = new QPushButton(Tr::tr("&Revert"));
    m_cherryPickButton = new QPushButton(Tr::tr("Cherry &Pick"));
    m_showButton = new QPushButton(Tr::tr("&Show"));
    m_showButton->setObjectName("showButton");

    m_workingDirectoryChooser->setExpectedKind(PathChooser::ExistingDirectory);
    m_workingDirectoryChooser->setPromptDialogTitle(Tr::tr("Select Git Directory"));
    m_workingDirectoryChooser->setFilePath(workingDirectory);

    using namespace Layouting;
    Column {
        Grid {
            Tr::tr("Working directory:"), m_workingDirectoryChooser, br,
            Tr::tr("Change:"), m_changeNumberEdit, selectFromHistoryButton,
        },
        m_detailsText,
        Row {
            closeButton, st, archiveButton, m_checkoutButton,
            m_revertButton, m_cherryPickButton, m_showButton
        }
    }.attachTo(this);

    connect(m_changeNumberEdit, &CompletingLineEdit::textChanged,
            this, &ChangeSelectionDialog::changeTextChanged);
    connect(m_workingDirectoryChooser, &PathChooser::textChanged,
            this, &ChangeSelectionDialog::recalculateDetails);
    connect(m_workingDirectoryChooser, &PathChooser::textChanged,
            this, &ChangeSelectionDialog::recalculateCompletion);
    connect(selectFromHistoryButton, &QPushButton::clicked,
            this, &ChangeSelectionDialog::selectCommitFromRecentHistory);
    connect(m_showButton, &QPushButton::clicked,
            this, std::bind(&ChangeSelectionDialog::acceptCommand, this, Show));
    connect(m_cherryPickButton, &QPushButton::clicked,
            this, std::bind(&ChangeSelectionDialog::acceptCommand, this, CherryPick));
    connect(m_revertButton, &QPushButton::clicked,
            this, std::bind(&ChangeSelectionDialog::acceptCommand, this, Revert));
    connect(m_checkoutButton, &QPushButton::clicked,
            this, std::bind(&ChangeSelectionDialog::acceptCommand, this, Checkout));
    connect(archiveButton, &QPushButton::clicked,
            this, std::bind(&ChangeSelectionDialog::acceptCommand, this, Archive));

    if (id == "Git.Revert")
        m_revertButton->setDefault(true);
    else if (id == "Git.CherryPick")
        m_cherryPickButton->setDefault(true);
    else if (id == "Git.Checkout")
        m_checkoutButton->setDefault(true);
    else if (id == "Git.Archive")
        archiveButton->setDefault(true);
    else
        m_showButton->setDefault(true);
    m_changeModel = new QStringListModel(this);
    auto changeCompleter = new QCompleter(m_changeModel, this);
    m_changeNumberEdit->setCompleter(changeCompleter);
    changeCompleter->setCaseSensitivity(Qt::CaseInsensitive);

    recalculateDetails();
    recalculateCompletion();

    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
}

ChangeSelectionDialog::~ChangeSelectionDialog() = default;

QString ChangeSelectionDialog::change() const
{
    return m_changeNumberEdit->text().trimmed();
}

void ChangeSelectionDialog::selectCommitFromRecentHistory()
{
    FilePath workingDir = workingDirectory();
    if (workingDir.isEmpty())
        return;

    QString commit = change();
    int tilde = commit.indexOf('~');
    if (tilde != -1)
        commit.truncate(tilde);
    LogChangeDialog dialog(false, this);
    dialog.setWindowTitle(Tr::tr("Select Commit"));

    dialog.runDialog(workingDir, commit, LogChangeWidget::IncludeRemotes);

    if (dialog.result() == QDialog::Rejected || dialog.commitIndex() == -1)
        return;

    m_changeNumberEdit->setText(dialog.commit());
}

FilePath ChangeSelectionDialog::workingDirectory() const
{
    const FilePath workingDir = m_workingDirectoryChooser->filePath();
    if (workingDir.isEmpty() || !workingDir.exists())
        return {};

    return Core::VcsManager::findTopLevelForDirectory(workingDir);
}

ChangeCommand ChangeSelectionDialog::command() const
{
    return m_command;
}

void ChangeSelectionDialog::acceptCommand(ChangeCommand command)
{
    m_command = command;
    QDialog::accept();
}

//! Set commit message in details
void ChangeSelectionDialog::setDetails()
{
    Theme *theme = creatorTheme();

    QPalette palette;
    if (m_process->result() == ProcessResult::FinishedWithSuccess) {
        m_detailsText->setPlainText(m_process->cleanedStdOut());
        palette.setColor(QPalette::Text, theme->color(Theme::TextColorNormal));
        m_changeNumberEdit->setPalette(palette);
    } else if (m_process->result() == ProcessResult::StartFailed) {
        m_detailsText->setPlainText(Tr::tr("Error: Could not start Git."));
    } else {
        m_detailsText->setPlainText(Tr::tr("Error: Unknown reference"));
        palette.setColor(QPalette::Text, theme->color(Theme::TextColorError));
        m_changeNumberEdit->setPalette(palette);
        enableButtons(false);
    }
}

void ChangeSelectionDialog::enableButtons(bool b)
{
    m_showButton->setEnabled(b);
    m_cherryPickButton->setEnabled(b);
    m_revertButton->setEnabled(b);
    m_checkoutButton->setEnabled(b);
}

void ChangeSelectionDialog::recalculateCompletion()
{
    const FilePath workingDir = workingDirectory();
    if (workingDir == m_oldWorkingDir)
        return;
    m_oldWorkingDir = workingDir;
    m_changeModel->setStringList(QStringList());

    if (workingDir.isEmpty())
        return;

    Process *process = new Process(this);
    process->setEnvironment(gitClient().processEnvironment());
    process->setCommand({gitClient().vcsBinary(), {"for-each-ref", "--format=%(refname:short)"}});
    process->setWorkingDirectory(workingDir);
    process->setUseCtrlCStub(true);
    connect(process, &Process::done, this, [this, process] {
        if (process->result() == ProcessResult::FinishedWithSuccess)
            m_changeModel->setStringList(process->cleanedStdOut().split('\n'));
        process->deleteLater();
    });
    process->start();
}

void ChangeSelectionDialog::recalculateDetails()
{
    enableButtons(true);

    const FilePath workingDir = workingDirectory();
    if (workingDir.isEmpty()) {
        m_detailsText->setPlainText(Tr::tr("Error: Bad working directory."));
        return;
    }

    const QString ref = change();
    if (ref.isEmpty()) {
        m_detailsText->clear();
        return;
    }

    m_process.reset(new Process);
    connect(m_process.get(), &Process::done, this, &ChangeSelectionDialog::setDetails);
    m_process->setWorkingDirectory(workingDir);
    m_process->setEnvironment(m_gitEnvironment);
    m_process->setCommand({m_gitExecutable, {"show", "--decorate", "--stat=80", ref}});
    m_process->start();
    m_detailsText->setPlainText(Tr::tr("Fetching commit data..."));
}

void ChangeSelectionDialog::changeTextChanged(const QString &text)
{
    if (QCompleter *comp = m_changeNumberEdit->completer()) {
        if (text.isEmpty() && !comp->popup()->isVisible()) {
            comp->setCompletionPrefix(text);
            QTimer::singleShot(0, comp, [comp]{ comp->complete(); });
        }
    }
    recalculateDetails();
}

} // Git::Internal
