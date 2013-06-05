/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <QProcess>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QDir>
#include <QFileDialog>

namespace Git {
namespace Internal {

ChangeSelectionDialog::ChangeSelectionDialog(const QString &workingDirectory, QWidget *parent)
    : QDialog(parent)
    , m_process(0)
    , m_workingDirEdit(new QLineEdit(workingDirectory, this))
    , m_changeNumberEdit(new QLineEdit(this))
    , m_selectDirButton(new QPushButton(tr("Browse &Directory..."), this))
    , m_selectFromHistoryButton(new QPushButton(tr("Browse &History..."), this))
    , m_showButton(new QPushButton(tr("&Show"), this))
    , m_cherryPickButton(new QPushButton(tr("Cherry &Pick"), this))
    , m_revertButton(new QPushButton(tr("&Revert"), this))
    , m_checkoutButton(new QPushButton(tr("Check&out"), this))
    , m_closeButton(new QPushButton(tr("&Close"), this))
    , m_detailsText(new QPlainTextEdit(this))
    , m_command(NoCommand)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    bool ok;
    m_gitBinaryPath = GitPlugin::instance()->gitClient()->gitBinaryPath(&ok);
    if (!ok)
        return;

    QGridLayout* layout = new QGridLayout(this);
    layout->addWidget(new QLabel(tr("Working directory:"), this), 0, 0 , 1, 1);
    layout->addWidget(m_workingDirEdit, 0, 1, 1, 1);
    layout->addWidget(m_selectDirButton, 0, 2, 1, 1);
    layout->addWidget(new QLabel(tr("Change:"), this),1, 0, 1, 1);
    layout->addWidget(m_changeNumberEdit, 1, 1, 1, 1);
    layout->addWidget(m_selectFromHistoryButton, 1, 2, 1, 1);
    layout->addWidget(m_detailsText, 2, 0, 1, 3);

    QHBoxLayout* buttonsLine = new QHBoxLayout();
    buttonsLine->addWidget(m_closeButton);
    buttonsLine->addStretch();
    buttonsLine->addWidget(m_checkoutButton);
    buttonsLine->addWidget(m_revertButton);
    buttonsLine->addWidget(m_cherryPickButton);
    buttonsLine->addWidget(m_showButton);

    layout->addLayout(buttonsLine, 3, 0 ,1 ,3);

    m_changeNumberEdit->setFocus(Qt::ActiveWindowFocusReason);
    m_changeNumberEdit->setText(QLatin1String("HEAD"));
    m_changeNumberEdit->selectAll();

    setWindowTitle(tr("Select a Git Commit"));
    setMinimumSize(QSize(550, 350));

    m_gitEnvironment = GitPlugin::instance()->gitClient()->processEnvironment();
    connect(m_changeNumberEdit, SIGNAL(textChanged(QString)),
            this, SLOT(recalculateDetails()));
    connect(m_workingDirEdit, SIGNAL(textChanged(QString)),
            this, SLOT(recalculateDetails()));
    connect(m_selectFromHistoryButton, SIGNAL(clicked()),
            this, SLOT(selectCommitFromRecentHistory()));
    connect(m_selectDirButton, SIGNAL(clicked()), this, SLOT(chooseWorkingDirectory()));

    connect(m_showButton, SIGNAL(clicked()), this, SLOT(acceptShow()));
    connect(m_cherryPickButton, SIGNAL(clicked()), this, SLOT(acceptCherryPick()));
    connect(m_revertButton, SIGNAL(clicked()), this, SLOT(acceptRevert()));
    connect(m_checkoutButton, SIGNAL(clicked()), this, SLOT(acceptCheckout()));
    connect(m_closeButton, SIGNAL(clicked()), this, SLOT(reject()));

    recalculateDetails();
}

ChangeSelectionDialog::~ChangeSelectionDialog()
{
    delete m_process;
}

QString ChangeSelectionDialog::change() const
{
    return m_changeNumberEdit->text();
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

    dialog.runDialog(workingDir, commit);

    if (dialog.result() == QDialog::Rejected || dialog.commitIndex() == -1)
        return;

    if (dialog.commitIndex() > 0)
        commit += QLatin1Char('~') + QString::number(dialog.commitIndex());

    m_changeNumberEdit->setText(commit);
}

void ChangeSelectionDialog::chooseWorkingDirectory()
{
    QString folder = QFileDialog::getExistingDirectory(this, tr("Select Git Directory"),
                                                       m_workingDirEdit->text());

    if (folder.isEmpty())
        return;

    m_workingDirEdit->setText(folder);
}

QString ChangeSelectionDialog::workingDirectory() const
{
    if (m_workingDirEdit->text().isEmpty() || !QDir(m_workingDirEdit->text()).exists())
        return QString();

    return GitPlugin::instance()->gitClient()->
            findRepositoryForDirectory(m_workingDirEdit->text());
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
    QPalette palette = m_changeNumberEdit->palette();
    if (exitCode == 0) {
        m_detailsText->setPlainText(QString::fromUtf8(m_process->readAllStandardOutput()));
        palette.setColor(QPalette::Text, Qt::black);
        m_changeNumberEdit->setPalette(palette);
        enableButtons(true);
    } else {
        m_detailsText->setPlainText(tr("Error: Unknown reference"));
        palette.setColor(QPalette::Text, Qt::red);
        m_changeNumberEdit->setPalette(palette);
    }
}

void ChangeSelectionDialog::enableButtons(bool b)
{
    m_showButton->setEnabled(b);
    m_cherryPickButton->setEnabled(b);
    m_revertButton->setEnabled(b);
    m_checkoutButton->setEnabled(b);
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

    QString workingDir = workingDirectory();
    QPalette palette = m_workingDirEdit->palette();
    if (workingDir.isEmpty()) {
        m_detailsText->setPlainText(tr("Error: Bad working directory."));
        palette.setColor(QPalette::Text, Qt::red);
        m_workingDirEdit->setPalette(palette);
        return;
    } else {
        palette.setColor(QPalette::Text, Qt::black);
        m_workingDirEdit->setPalette(palette);
    }

    QStringList args;
    args << QLatin1String("log") << QLatin1String("-n1") << m_changeNumberEdit->text();

    m_process = new QProcess(this);
    m_process->setWorkingDirectory(workingDir);
    m_process->setProcessEnvironment(m_gitEnvironment);

    connect(m_process, SIGNAL(finished(int)), this, SLOT(setDetails(int)));

    m_process->start(m_gitBinaryPath, args);
    m_process->closeWriteChannel();
    if (!m_process->waitForStarted())
        m_detailsText->setPlainText(tr("Error: Could not start Git."));
    else
        m_detailsText->setPlainText(tr("Fetching commit data..."));
}

} // Internal
} // Git
