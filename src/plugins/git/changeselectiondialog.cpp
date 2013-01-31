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
#include "gitplugin.h"
#include "gitclient.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>

namespace Git {
namespace Internal {

ChangeSelectionDialog::ChangeSelectionDialog(const QString &workingDirectory, QWidget *parent)
    : QDialog(parent)
    , m_process(0)
{
    m_ui.setupUi(this);
    if (!workingDirectory.isEmpty()) {
        setWorkingDirectory(workingDirectory);
        m_ui.workingDirectoryButton->setEnabled(false);
        m_ui.workingDirectoryEdit->setEnabled(false);
    }
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    connect(m_ui.workingDirectoryButton, SIGNAL(clicked()), this, SLOT(selectWorkingDirectory()));
    setWindowTitle(tr("Select a Git Commit"));

    bool ok;
    m_gitBinaryPath = GitPlugin::instance()->gitClient()->gitBinaryPath(&ok);
    if (!ok)
        return;

    m_gitEnvironment = GitPlugin::instance()->gitClient()->processEnvironment();
    connect(m_ui.changeNumberEdit, SIGNAL(textChanged(QString)),
            this, SLOT(recalculateDetails(QString)));
    recalculateDetails(m_ui.changeNumberEdit->text());
}

ChangeSelectionDialog::~ChangeSelectionDialog()
{
    delete m_process;
}

QString ChangeSelectionDialog::change() const
{
    return m_ui.changeNumberEdit->text();
}

QString ChangeSelectionDialog::workingDirectory() const
{
    return m_ui.workingDirectoryEdit->text();
}

void ChangeSelectionDialog::setWorkingDirectory(const QString &s)
{
    if (s.isEmpty())
        return;
    m_ui.workingDirectoryEdit->setText(QDir::toNativeSeparators(s));
    m_ui.changeNumberEdit->setFocus(Qt::ActiveWindowFocusReason);
    m_ui.changeNumberEdit->setText(QLatin1String("HEAD"));
    m_ui.changeNumberEdit->selectAll();
}

void ChangeSelectionDialog::selectWorkingDirectory()
{
    QString location = QFileDialog::getExistingDirectory(this, tr("Select Working Directory"),
                                                         m_ui.workingDirectoryEdit->text());
    if (location.isEmpty())
        return;

    // Verify that the location is a repository
    // We allow specifying a directory, which is not the head directory of the repository.
    // This is useful for git show commit:./file
    QString topLevel = GitPlugin::instance()->gitClient()->findRepositoryForDirectory(location);
    if (!topLevel.isEmpty())
        m_ui.workingDirectoryEdit->setText(location);
    else // Did not find a repo
        QMessageBox::critical(this, tr("Error"),
                              tr("Selected directory is not a Git repository"));
}

//! Set commit message in details
void ChangeSelectionDialog::setDetails(int exitCode)
{
    if (exitCode == 0)
        m_ui.detailsText->setPlainText(QString::fromUtf8(m_process->readAllStandardOutput()));
    else
        m_ui.detailsText->setPlainText(tr("Error: unknown reference"));
}

void ChangeSelectionDialog::recalculateDetails(const QString &ref)
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished();
        delete m_process;
    }

    QStringList args;
    args << QLatin1String("log") << QLatin1String("-n1") << ref;

    m_process = new QProcess(this);
    m_process->setWorkingDirectory(workingDirectory());
    m_process->setProcessEnvironment(m_gitEnvironment);

    connect(m_process, SIGNAL(finished(int)), this, SLOT(setDetails(int)));

    m_process->start(m_gitBinaryPath, args);
    m_process->closeWriteChannel();
    if (!m_process->waitForStarted())
        m_ui.detailsText->setPlainText(tr("Error: could not start git"));
    else
        m_ui.detailsText->setPlainText(tr("Fetching commit data..."));
}

} // Internal
} // Git
