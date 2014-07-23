/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "changeselectiondialog.h"
#include "logchangedialog.h"
#include "gitplugin.h"
#include "gitclient.h"
#include "ui_changeselectiondialog.h"

#include <coreplugin/vcsmanager.h>

#include <QProcess>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QDir>
#include <QFileDialog>
#include <QCompleter>
#include <QStringListModel>
#include <QTimer>

namespace Git {
namespace Internal {

ChangeSelectionDialog::ChangeSelectionDialog(const QString &workingDirectory, Core::Id id, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::ChangeSelectionDialog)
    , m_process(0)
    , m_command(NoCommand)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_gitExecutable = GitPlugin::instance()->gitClient()->gitExecutable();
    m_ui->setupUi(this);
    m_ui->workingDirectoryEdit->setText(workingDirectory);
    m_gitEnvironment = GitPlugin::instance()->gitClient()->processEnvironment();
    m_ui->changeNumberEdit->setFocus();
    m_ui->changeNumberEdit->selectAll();

    connect(m_ui->changeNumberEdit, SIGNAL(textChanged(QString)), this, SLOT(changeTextChanged(QString)));
    connect(m_ui->workingDirectoryEdit, SIGNAL(textChanged(QString)), this, SLOT(recalculateDetails()));
    connect(m_ui->workingDirectoryEdit, SIGNAL(textChanged(QString)), this, SLOT(recalculateCompletion()));
    connect(m_ui->selectDirectoryButton, SIGNAL(clicked()), this, SLOT(chooseWorkingDirectory()));
    connect(m_ui->selectFromHistoryButton, SIGNAL(clicked()), this, SLOT(selectCommitFromRecentHistory()));
    connect(m_ui->showButton, SIGNAL(clicked()), this, SLOT(acceptShow()));
    connect(m_ui->cherryPickButton, SIGNAL(clicked()), this, SLOT(acceptCherryPick()));
    connect(m_ui->revertButton, SIGNAL(clicked()), this, SLOT(acceptRevert()));
    connect(m_ui->checkoutButton, SIGNAL(clicked()), this, SLOT(acceptCheckout()));

    if (id == "Git.Revert")
        m_ui->revertButton->setDefault(true);
    else if (id == "Git.CherryPick")
        m_ui->cherryPickButton->setDefault(true);
    else if (id == "Git.Checkout")
        m_ui->checkoutButton->setDefault(true);
    else
        m_ui->showButton->setDefault(true);
    m_changeModel = new QStringListModel(this);
    QCompleter *changeCompleter = new QCompleter(m_changeModel, this);
    m_ui->changeNumberEdit->setCompleter(changeCompleter);
    changeCompleter->setCaseSensitivity(Qt::CaseInsensitive);

    recalculateDetails();
    recalculateCompletion();
}

ChangeSelectionDialog::~ChangeSelectionDialog()
{
    delete m_ui;
    delete m_process;
}

QString ChangeSelectionDialog::change() const
{
    return m_ui->changeNumberEdit->text().trimmed();
}

void ChangeSelectionDialog::selectCommitFromRecentHistory()
{
    QString workingDir = workingDirectory();
    if (workingDir.isEmpty())
        return;

    QString commit = change();
    int tilde = commit.indexOf(QLatin1Char('~'));
    if (tilde != -1)
        commit.truncate(tilde);
    LogChangeDialog dialog(false, this);
    dialog.setWindowTitle(tr("Select Commit"));

    dialog.runDialog(workingDir, commit, LogChangeWidget::IncludeRemotes);

    if (dialog.result() == QDialog::Rejected || dialog.commitIndex() == -1)
        return;

    if (dialog.commitIndex() > 0)
        commit += QLatin1Char('~') + QString::number(dialog.commitIndex());

    m_ui->changeNumberEdit->setText(commit);
}

void ChangeSelectionDialog::chooseWorkingDirectory()
{
    QString folder = QFileDialog::getExistingDirectory(this, tr("Select Git Directory"),
                                                       m_ui->workingDirectoryEdit->text());

    if (folder.isEmpty())
        return;

    m_ui->workingDirectoryEdit->setText(folder);
}

QString ChangeSelectionDialog::workingDirectory() const
{
    const QString workingDir = m_ui->workingDirectoryEdit->text();
    if (workingDir.isEmpty() || !QDir(workingDir).exists())
        return QString();

    return Core::VcsManager::findTopLevelForDirectory(workingDir);
}

ChangeCommand ChangeSelectionDialog::command() const
{
    return m_command;
}

void ChangeSelectionDialog::acceptCheckout()
{
    m_command = Checkout;
    accept();
}

void ChangeSelectionDialog::acceptCherryPick()
{
    m_command = CherryPick;
    accept();
}

void ChangeSelectionDialog::acceptRevert()
{
    m_command = Revert;
    accept();
}

void ChangeSelectionDialog::acceptShow()
{
    m_command = Show;
    accept();
}

//! Set commit message in details
void ChangeSelectionDialog::setDetails(int exitCode)
{
    QPalette palette;
    if (exitCode == 0) {
        m_ui->detailsText->setPlainText(QString::fromUtf8(m_process->readAllStandardOutput()));
        palette.setColor(QPalette::Text, Qt::black);
        m_ui->changeNumberEdit->setPalette(palette);
        enableButtons(true);
    } else {
        m_ui->detailsText->setPlainText(tr("Error: Unknown reference"));
        palette.setColor(QPalette::Text, Qt::red);
        m_ui->changeNumberEdit->setPalette(palette);
    }
}

void ChangeSelectionDialog::enableButtons(bool b)
{
    m_ui->showButton->setEnabled(b);
    m_ui->cherryPickButton->setEnabled(b);
    m_ui->revertButton->setEnabled(b);
    m_ui->checkoutButton->setEnabled(b);
}

void ChangeSelectionDialog::recalculateCompletion()
{
    const QString workingDir = workingDirectory();
    if (workingDir == m_oldWorkingDir)
        return;
    m_oldWorkingDir = workingDir;

    if (!workingDir.isEmpty()) {
        GitClient *client = GitPlugin::instance()->gitClient();
        QStringList args;
        args << QLatin1String("--format=%(refname:short)");
        QString output;
        if (client->synchronousForEachRefCmd(workingDir, args, &output)) {
            m_changeModel->setStringList(output.split(QLatin1Char('\n')));
            return;
        }
    }
    m_changeModel->setStringList(QStringList());
}

void ChangeSelectionDialog::recalculateDetails()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished();
        delete m_process;
        m_process = 0;
    }
    enableButtons(false);

    const QString workingDir = workingDirectory();
    QPalette palette = m_ui->workingDirectoryEdit->palette();
    if (workingDir.isEmpty()) {
        m_ui->detailsText->setPlainText(tr("Error: Bad working directory."));
        palette.setColor(QPalette::Text, Qt::red);
        m_ui->workingDirectoryEdit->setPalette(palette);
        return;
    } else {
        palette.setColor(QPalette::Text, Qt::black);
        m_ui->workingDirectoryEdit->setPalette(palette);
    }

    const QString ref = change();
    if (ref.isEmpty()) {
        m_ui->detailsText->clear();
        return;
    }

    QStringList args;
    args << QLatin1String("log") << QLatin1String("-n1") << ref;

    m_process = new QProcess(this);
    m_process->setWorkingDirectory(workingDir);
    m_process->setProcessEnvironment(m_gitEnvironment);

    connect(m_process, SIGNAL(finished(int)), this, SLOT(setDetails(int)));

    m_process->start(m_gitExecutable.toString(), args);
    m_process->closeWriteChannel();
    if (!m_process->waitForStarted())
        m_ui->detailsText->setPlainText(tr("Error: Could not start Git."));
    else
        m_ui->detailsText->setPlainText(tr("Fetching commit data..."));
}

void ChangeSelectionDialog::changeTextChanged(const QString &text)
{
    if (QCompleter *comp = m_ui->changeNumberEdit->completer()) {
        if (text.isEmpty() && !comp->popup()->isVisible()) {
            comp->setCompletionPrefix(text);
            QTimer::singleShot(0, comp, SLOT(complete()));
        }
    }
    recalculateDetails();
}

} // Internal
} // Git
