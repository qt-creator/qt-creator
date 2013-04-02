/****************************************************************************
**
** Copyright (C) 2013 Petar Perisin.
** Contact: petar.perisin@gmail.com
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

#include "gerritpushdialog.h"
#include "ui_gerritpushdialog.h"

#include "../gitplugin.h"
#include "../gitclient.h"
#include <coreplugin/icore.h>

#include <QDir>
#include <QMessageBox>

namespace Gerrit {
namespace Internal {

GerritPushDialog::GerritPushDialog(const QString &workingDir, QWidget *parent) :
    QDialog(parent),
    m_workingDir(workingDir),
    m_ui(new Ui::GerritPushDialog),
    m_remoteBranches(new QMap<QString,QString>()),
    m_localChangesFound(true)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->repositoryLabel->setText(tr("Local Repository: %1").arg(
                                       QDir::toNativeSeparators(workingDir)));

    Git::Internal::GitClient *gitClient = Git::Internal::GitPlugin::instance()->gitClient();
    QString output;
    QString error;
    QStringList args;

    args << QLatin1String("--no-color") << QLatin1String("--format=%P")
         << QLatin1String("HEAD") << QLatin1String("--not")<< QLatin1String("--remotes");

    if (!gitClient->synchronousLog(m_workingDir, args, &output) || output.isEmpty())
        reject();

    output.chop(1);
    if (output.isEmpty()) {
        output = QLatin1String("HEAD");
        m_localChangesFound = false;
    } else {
        output = output.mid(output.lastIndexOf(QLatin1Char('\n')) + 1);
    }

    args.clear();
    args << QLatin1String("--remotes") << QLatin1String("--contains") << output;

    if (!gitClient->synchronousBranchCmd(m_workingDir, args, &output, &error))
        reject();

    QString head = QLatin1String("/HEAD");
    QStringList refs = output.split(QLatin1Char('\n'));
    foreach (const QString &reference, refs) {
        if (reference.contains(head) || reference.isEmpty())
            continue;

        m_suggestedRemoteName = reference.left(reference.indexOf(QLatin1Char('/'))).trimmed();
        m_suggestedRemoteBranch = reference.mid(reference.indexOf(QLatin1Char('/')) + 1).trimmed();
        break;
    }

    output.clear();
    error.clear();
    args.clear();

    args << QLatin1String("--remotes");

    if (!gitClient->synchronousBranchCmd(m_workingDir, args, &output, &error))
        reject();

    refs.clear();
    refs = output.split(QLatin1String("\n"));
    foreach (const QString &reference, refs) {
        if (reference.contains(head) || reference.isEmpty())
            continue;

        int refBranchIndex = reference.indexOf(QLatin1Char('/'));
        m_remoteBranches->insertMulti(reference.left(refBranchIndex).trimmed(),
                                     reference.mid(refBranchIndex + 1).trimmed());
    }

    int currIndex = 0;
    QStringList remotes = m_remoteBranches->keys();
    remotes.removeDuplicates();
    foreach (const QString &remote, remotes) {
        m_ui->remoteComboBox->addItem(remote);
        if (remote == m_suggestedRemoteName)
            m_ui->remoteComboBox->setCurrentIndex(currIndex);
        ++currIndex;
    }
    if (m_ui->remoteComboBox->count() < 1)
        reject();

    m_ui->remoteComboBox->setEnabled(m_ui->remoteComboBox->count() != 1);

    connect(m_ui->remoteComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setRemoteBranches()));
    connect(m_ui->branchComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setChangeRange()));
    setRemoteBranches();
}

GerritPushDialog::~GerritPushDialog()
{
    delete m_ui;
    delete m_remoteBranches;
}


QString GerritPushDialog::calculateChangeRange()
{
    QString remote = selectedRemoteName();
    remote += QLatin1Char('/');
    remote += selectedRemoteBranchName();

    QStringList args(remote + QLatin1String("..HEAD"));
    args << QLatin1String("--count");

    QString number;

    if (!Git::Internal::GitPlugin::instance()->gitClient()->
            synchronousRevListCmd(m_workingDir, args, &number))
        reject();

    number.chop(1);
    return number;
}

void GerritPushDialog::setChangeRange()
{
    QString remote = selectedRemoteName();
    remote += QLatin1Char('/');
    remote += selectedRemoteBranchName();
    m_ui->infoLabel->setText(tr("Number of commits between HEAD and %1: %2").arg(
                                 remote, calculateChangeRange()));
}

bool GerritPushDialog::localChangesFound() const
{
    return m_localChangesFound;
}

void GerritPushDialog::setRemoteBranches()
{
    m_ui->branchComboBox->clear();

    QMap<QString, QString>::const_iterator it;
    int i = 0;
    for (it = m_remoteBranches->constBegin(); it != m_remoteBranches->constEnd(); ++it) {
        if (it.key() == selectedRemoteName()) {
            m_ui->branchComboBox->addItem(it.value());
            if (it.value() == m_suggestedRemoteBranch)
                m_ui->branchComboBox->setCurrentIndex(i);
            ++i;
        }
    }
    setChangeRange();
}

QString GerritPushDialog::selectedRemoteName() const
{
    return m_ui->remoteComboBox->currentText();
}

QString GerritPushDialog::selectedRemoteBranchName() const
{
    return m_ui->branchComboBox->currentText();
}

QString GerritPushDialog::selectedPushType() const
{
    return m_ui->publicRadioButton->isChecked() ? QLatin1String("for") : QLatin1String("draft");
}

} // namespace Internal
} // namespace Gerrit
