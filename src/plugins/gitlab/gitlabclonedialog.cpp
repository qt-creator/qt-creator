// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitlabclonedialog.h"

#include "gitlabprojectsettings.h"
#include "gitlabtr.h"
#include "resultparser.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/vcsmanager.h>

#include <git/gitclient.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/fancylineedit.h>
#include <utils/filepath.h>
#include <utils/infolabel.h>
#include <utils/mimeutils.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcscommand.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Utils;
using namespace VcsBase;

namespace GitLab {

GitLabCloneDialog::GitLabCloneDialog(const Project &project, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(Tr::tr("Clone Repository"));
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(Tr::tr("Specify repository URL, checkout path and directory.")));
    QHBoxLayout *centerLayout = new QHBoxLayout;
    QFormLayout *form = new QFormLayout;
    m_repositoryCB = new QComboBox(this);
    m_repositoryCB->addItems({project.sshUrl, project.httpUrl});
    form->addRow(Tr::tr("Repository"), m_repositoryCB);
    m_pathChooser = new PathChooser(this);
    m_pathChooser->setExpectedKind(PathChooser::ExistingDirectory);
    form->addRow(Tr::tr("Path"), m_pathChooser);
    m_directoryLE = new FancyLineEdit(this);
    m_directoryLE->setValidationFunction([this](FancyLineEdit *e, QString *msg) {
        const FilePath fullPath = m_pathChooser->filePath().pathAppended(e->text());
        bool alreadyExists = fullPath.exists();
        if (alreadyExists && msg)
            *msg = Tr::tr("Path \"%1\" already exists.").arg(fullPath.toUserOutput());
        return !alreadyExists;
    });
    form->addRow(Tr::tr("Directory"), m_directoryLE);
    m_submodulesCB = new QCheckBox(this);
    form->addRow(Tr::tr("Recursive"), m_submodulesCB);
    form->addItem(new QSpacerItem(10, 10));
    centerLayout->addLayout(form);
    m_cloneOutput = new QPlainTextEdit(this);
    m_cloneOutput->setReadOnly(true);
    centerLayout->addWidget(m_cloneOutput);
    layout->addLayout(centerLayout);
    m_infoLabel = new InfoLabel(this);
    layout->addWidget(m_infoLabel);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_cloneButton = new QPushButton(Tr::tr("Clone"), this);
    buttons->addButton(m_cloneButton, QDialogButtonBox::ActionRole);
    m_cancelButton = buttons->button(QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    setLayout(layout);

    m_pathChooser->setFilePath(Core::DocumentManager::projectsDirectory());
    auto [host, path, port]
            = GitLabProjectSettings::remotePartsFromRemote(m_repositoryCB->currentText());
    int slashIndex = path.indexOf('/');
    QTC_ASSERT(slashIndex > 0, return);
    m_directoryLE->setText(path.mid(slashIndex + 1));

    connect(m_pathChooser, &PathChooser::textChanged, this, [this] {
        m_directoryLE->validate();
        updateUi();
    });
    connect(m_pathChooser, &PathChooser::validChanged, this, &GitLabCloneDialog::updateUi);
    connect(m_directoryLE, &FancyLineEdit::textChanged, this, &GitLabCloneDialog::updateUi);
    connect(m_directoryLE, &FancyLineEdit::validChanged, this, &GitLabCloneDialog::updateUi);
    connect(m_cloneButton, &QPushButton::clicked, this, &GitLabCloneDialog::cloneProject);
    connect(m_cancelButton, &QPushButton::clicked,
            this, &GitLabCloneDialog::cancel);
    connect(this, &QDialog::rejected, this, [this]() {
        if (m_commandRunning) {
            cancel();
            QApplication::restoreOverrideCursor();
            return;
        }
    });

    updateUi();
    resize(575, 265);
}

void GitLabCloneDialog::updateUi()
{
    bool pathValid = m_pathChooser->isValid();
    bool directoryValid = m_directoryLE->isValid();
    m_cloneButton->setEnabled(pathValid && directoryValid);
    if (!pathValid) {
        m_infoLabel->setText(m_pathChooser->errorMessage());
        m_infoLabel->setType(InfoLabel::Error);
    } else if (!directoryValid) {
        m_infoLabel->setText(m_directoryLE->errorMessage());
        m_infoLabel->setType(InfoLabel::Error);
    }
    m_infoLabel->setVisible(!pathValid || !directoryValid);
}

void GitLabCloneDialog::cloneProject()
{
    VcsBasePluginPrivate *vc = static_cast<VcsBasePluginPrivate *>(
                Core::VcsManager::versionControl(Id::fromString("G.Git")));
    QTC_ASSERT(vc, return);
    const QStringList extraArgs = m_submodulesCB->isChecked() ? QStringList{ "--recursive" }
                                                              : QStringList{};
    m_command = vc->createInitialCheckoutCommand(m_repositoryCB->currentText(),
                                                 m_pathChooser->absoluteFilePath(),
                                                 m_directoryLE->text(), extraArgs);
    const FilePath workingDirectory = m_pathChooser->absoluteFilePath();
    m_command->addFlags(RunFlags::ProgressiveOutput);
    connect(m_command, &VcsCommand::stdOutText, this, [this](const QString &text) {
        m_cloneOutput->appendPlainText(text);
    });
    connect(m_command, &VcsCommand::stdErrText, this, [this](const QString &text) {
        m_cloneOutput->appendPlainText(text);
    });
    connect(m_command, &VcsCommand::done, this, [this] {
        cloneFinished(m_command->result() == ProcessResult::FinishedWithSuccess);
    });
    QApplication::setOverrideCursor(Qt::WaitCursor);

    m_cloneOutput->clear();
    m_cloneButton->setEnabled(false);
    m_pathChooser->setReadOnly(true);
    m_directoryLE->setReadOnly(true);
    m_commandRunning = true;
    m_command->start();
}

void GitLabCloneDialog::cancel()
{
    if (m_commandRunning) {
        m_cloneOutput->appendPlainText(Tr::tr("User canceled process."));
        m_cancelButton->setEnabled(false);
        m_command->cancel();    // FIXME does not cancel the git processes... QTCREATORBUG-27567
    } else {
        reject();
    }
}

static FilePaths scanDirectoryForFiles(const FilePath &directory)
{
    FilePaths result;
    const FilePaths entries = directory.dirEntries(QDir::AllEntries | QDir::NoDotAndDotDot);

    for (const FilePath &entry : entries) {
        if (entry.isDir())
            result.append(scanDirectoryForFiles(entry));
        else
            result.append(entry);
    }
    return result;
}

void GitLabCloneDialog::cloneFinished(bool success)
{
    m_commandRunning = false;
    delete m_command;
    m_command = nullptr;

    const QString emptyLine("\n\n");
    m_cloneOutput->appendPlainText(emptyLine);
    QApplication::restoreOverrideCursor();

    if (success) {
        m_cloneOutput->appendPlainText(Tr::tr("Cloning succeeded.") + emptyLine);
        m_cloneButton->setEnabled(false);

        const FilePath base = m_pathChooser->filePath().pathAppended(m_directoryLE->text());
        FilePaths filesWeMayOpen = filtered(scanDirectoryForFiles(base), [](const FilePath &f) {
            return ProjectExplorer::ProjectManager::canOpenProjectForMimeType(mimeTypeForFile(f));
        });

        // limit the files to the most top-level item(s)
        int minimum = std::numeric_limits<int>::max();
        for (const FilePath &f : filesWeMayOpen) {
            int parentCount = f.toString().count('/');
            if (parentCount < minimum)
                minimum = parentCount;
        }
        filesWeMayOpen = filtered(filesWeMayOpen, [minimum](const FilePath &f) {
            return f.toString().count('/') == minimum;
        });

        hide(); // avoid to many dialogs.. FIXME: maybe change to some wizard approach?
        if (filesWeMayOpen.isEmpty()) {
            QMessageBox::warning(this, Tr::tr("Warning"),
                                 Tr::tr("Cloned project does not have a project file that can be "
                                    "opened. Try importing the project as a generic project."));
            accept();
        } else {
            const QStringList pFiles = Utils::transform(filesWeMayOpen, [base](const FilePath &f) {
                return f.relativePathFrom(base).toUserOutput();
            });
            bool ok = false;
            const QString fileToOpen
                    = QInputDialog::getItem(this, Tr::tr("Open Project"),
                                            Tr::tr("Choose the project file to be opened."),
                                            pFiles, 0, false, &ok);
            accept();
            if (ok && !fileToOpen.isEmpty())
                ProjectExplorer::ProjectExplorerPlugin::openProject(base.pathAppended(fileToOpen));
        }
    } else {
        m_cloneOutput->appendPlainText(Tr::tr("Cloning failed.") + emptyLine);
        const FilePath fullPath = m_pathChooser->filePath().pathAppended(m_directoryLE->text());
        fullPath.removeRecursively();
        m_cloneButton->setEnabled(true);
        m_cancelButton->setEnabled(true);
        m_pathChooser->setReadOnly(false);
        m_directoryLE->setReadOnly(false);
        m_directoryLE->validate();
    }
}

} // namespace GitLab
