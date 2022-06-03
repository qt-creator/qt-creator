/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "gitlabclonedialog.h"

#include "gitlabprojectsettings.h"
#include "resultparser.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/shellcommand.h>
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
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

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

namespace GitLab {

GitLabCloneDialog::GitLabCloneDialog(const Project &project, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Clone Repository"));
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Specify repository URL, checkout path and directory.")));
    QHBoxLayout *centerLayout = new QHBoxLayout;
    QFormLayout *form = new QFormLayout;
    m_repositoryCB = new QComboBox(this);
    m_repositoryCB->addItems({project.sshUrl, project.httpUrl});
    form->addRow(tr("Repository"), m_repositoryCB);
    m_pathChooser = new Utils::PathChooser(this);
    m_pathChooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    form->addRow(tr("Path"), m_pathChooser);
    m_directoryLE = new Utils::FancyLineEdit(this);
    m_directoryLE->setValidationFunction([this](Utils::FancyLineEdit *e, QString *msg) {
        const Utils::FilePath fullPath = m_pathChooser->filePath().pathAppended(e->text());
        bool alreadyExists = fullPath.exists();
        if (alreadyExists && msg)
            *msg = tr("Path \"%1\" already exists.").arg(fullPath.toUserOutput());
        return !alreadyExists;
    });
    form->addRow(tr("Directory"), m_directoryLE);
    m_submodulesCB = new QCheckBox(this);
    form->addRow(tr("Recursive"), m_submodulesCB);
    form->addItem(new QSpacerItem(10, 10));
    centerLayout->addLayout(form);
    m_cloneOutput = new QPlainTextEdit(this);
    m_cloneOutput->setReadOnly(true);
    centerLayout->addWidget(m_cloneOutput);
    layout->addLayout(centerLayout);
    m_infoLabel = new Utils::InfoLabel(this);
    layout->addWidget(m_infoLabel);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_cloneButton = new QPushButton(tr("Clone"), this);
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

    connect(m_pathChooser, &Utils::PathChooser::pathChanged, this, [this]() {
        m_directoryLE->validate();
        GitLabCloneDialog::updateUi();
    });
    connect(m_directoryLE, &Utils::FancyLineEdit::textChanged, this, &GitLabCloneDialog::updateUi);
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
        m_infoLabel->setType(Utils::InfoLabel::Error);
    } else if (!directoryValid) {
        m_infoLabel->setText(m_directoryLE->errorMessage());
        m_infoLabel->setType(Utils::InfoLabel::Error);
    }
    m_infoLabel->setVisible(!pathValid || !directoryValid);
}

void GitLabCloneDialog::cloneProject()
{
    Core::IVersionControl *vc = Core::VcsManager::versionControl(Utils::Id::fromString("G.Git"));
    QTC_ASSERT(vc, return);
    const QStringList extraArgs = m_submodulesCB->isChecked() ? QStringList{ "--recursive" }
                                                              : QStringList{};
    m_command = vc->createInitialCheckoutCommand(m_repositoryCB->currentText(),
                                                 m_pathChooser->absoluteFilePath(),
                                                 m_directoryLE->text(), extraArgs);
    const Utils::FilePath workingDirectory = m_pathChooser->absoluteFilePath();
    m_command->setProgressiveOutput(true);
    connect(m_command, &Utils::ShellCommand::stdOutText, this, [this](const QString &text) {
        m_cloneOutput->appendPlainText(text);
    });
    connect(m_command, &Utils::ShellCommand::stdErrText, this, [this](const QString &text) {
        m_cloneOutput->appendPlainText(text);
    });
    connect(m_command, &Utils::ShellCommand::finished, this, &GitLabCloneDialog::cloneFinished);
    QApplication::setOverrideCursor(Qt::WaitCursor);

    m_cloneOutput->clear();
    m_cloneButton->setEnabled(false);
    m_pathChooser->setReadOnly(true);
    m_directoryLE->setReadOnly(true);
    m_commandRunning = true;
    m_command->execute();
}

void GitLabCloneDialog::cancel()
{
    if (m_commandRunning) {
        m_cloneOutput->appendPlainText(tr("User canceled process."));
        m_cancelButton->setEnabled(false);
        m_command->cancel();    // FIXME does not cancel the git processes... QTCREATORBUG-27567
    } else {
        reject();
    }
}

static Utils::FilePaths scanDirectoryForFiles(const Utils::FilePath &directory)
{
    Utils::FilePaths result;
    const Utils::FilePaths entries = directory.dirEntries(QDir::AllEntries | QDir::NoDotAndDotDot);

    for (const Utils::FilePath &entry : entries) {
        if (entry.isDir())
            result.append(scanDirectoryForFiles(entry));
        else
            result.append(entry);
    }
    return result;
}

void GitLabCloneDialog::cloneFinished(bool ok, int exitCode)
{
    const bool success = (ok && exitCode == 0);
    m_commandRunning = false;
    delete m_command;
    m_command = nullptr;

    const QString emptyLine("\n\n");
    m_cloneOutput->appendPlainText(emptyLine);
    QApplication::restoreOverrideCursor();

    if (success) {
        m_cloneOutput->appendPlainText(tr("Cloning succeeded.") + emptyLine);
        m_cloneButton->setEnabled(false);

        const Utils::FilePath base = m_pathChooser->filePath().pathAppended(m_directoryLE->text());
        Utils::FilePaths filesWeMayOpen
                = Utils::filtered(scanDirectoryForFiles(base), [](const Utils::FilePath &f) {
            return ProjectExplorer::ProjectManager::canOpenProjectForMimeType(
                        Utils::mimeTypeForFile(f));
        });

        // limit the files to the most top-level item(s)
        int minimum = std::numeric_limits<int>::max();
        for (const Utils::FilePath &f : filesWeMayOpen) {
            int parentCount = f.toString().count('/');
            if (parentCount < minimum)
                minimum = parentCount;
        }
        filesWeMayOpen = Utils::filtered(filesWeMayOpen, [minimum](const Utils::FilePath &f) {
            return f.toString().count('/') == minimum;
        });

        hide(); // avoid to many dialogs.. FIXME: maybe change to some wizard approach?
        if (filesWeMayOpen.isEmpty()) {
            QMessageBox::warning(this, tr("Warning"),
                                 tr("Cloned project does not have a project file that can be "
                                    "opened. Try importing the project as a generic project."));
            accept();
        } else {
            const QStringList pFiles = Utils::transform(filesWeMayOpen,
                                                        [base](const Utils::FilePath &f) {
                return f.relativePath(base).toUserOutput();
            });
            bool ok = false;
            const QString fileToOpen
                    = QInputDialog::getItem(this, tr("Open Project"),
                                            tr("Choose the project file to be opened."),
                                            pFiles, 0, false, &ok);
            accept();
            if (ok && !fileToOpen.isEmpty())
                ProjectExplorer::ProjectExplorerPlugin::openProject(base.pathAppended(fileToOpen));
        }
    } else {
        m_cloneOutput->appendPlainText(tr("Cloning failed.") + emptyLine);
        const Utils::FilePath fullPath = m_pathChooser->filePath()
                .pathAppended(m_directoryLE->text());
        fullPath.removeRecursively();
        m_cloneButton->setEnabled(true);
        m_cancelButton->setEnabled(true);
        m_pathChooser->setReadOnly(false);
        m_directoryLE->setReadOnly(false);
        m_directoryLE->validate();
    }
}

} // namespace GitLab
