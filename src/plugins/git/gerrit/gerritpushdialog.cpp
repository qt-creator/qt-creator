/****************************************************************************
**
** Copyright (C) 2014 Petar Perisin.
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

#include <QDateTime>
#include <QDir>
#include <QRegExpValidator>

namespace Gerrit {
namespace Internal {

GerritPushDialog::GerritPushDialog(const QString &workingDir, const QString &reviewerList, QWidget *parent) :
    QDialog(parent),
    m_workingDir(workingDir),
    m_ui(new Ui::GerritPushDialog),
    m_localChangesFound(false),
    m_valid(false)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->repositoryLabel->setText(tr("<b>Local repository:</b> %1").arg(
                                       QDir::toNativeSeparators(workingDir)));

    if (!m_ui->commitView->init(workingDir, QString(), false))
        return;

    QString earliestCommit = m_ui->commitView->earliestCommit();
    if (earliestCommit.isEmpty())
        return;

    m_localChangesFound = true;

    Git::Internal::GitClient *gitClient = Git::Internal::GitPlugin::instance()->gitClient();
    QString output;
    QString error;
    QStringList args;
    args << QLatin1String("-r") << QLatin1String("--contains")
         << earliestCommit + QLatin1Char('^');

    if (!gitClient->synchronousBranchCmd(m_workingDir, args, &output, &error))
        return;

    QString head = QLatin1String("/HEAD");
    QStringList refs = output.split(QLatin1Char('\n'));
    QString remoteTrackingBranch = gitClient->synchronousTrackingBranch(m_workingDir);
    QString remoteBranch;
    foreach (const QString &reference, refs) {
        const QString ref = reference.trimmed();
        if (ref.contains(head) || ref.isEmpty())
            continue;

        if (remoteBranch.isEmpty())
            remoteBranch = ref;

        // Prefer remote tracking branch if it exists and contains the latest remote commit
        if (ref == remoteTrackingBranch) {
            remoteBranch = ref;
            break;
        }
    }

    if (!remoteBranch.isEmpty()) {
        m_suggestedRemoteName = remoteBranch.left(remoteBranch.indexOf(QLatin1Char('/')));
        m_suggestedRemoteBranch = remoteBranch.mid(remoteBranch.indexOf(QLatin1Char('/')) + 1);
    }

    output.clear();
    error.clear();
    args.clear();

    QString remotesPrefix(QLatin1String("refs/remotes/"));
    args << QLatin1String("--format=%(refname)\t%(committerdate:raw)")
         << remotesPrefix;
    if (!gitClient->synchronousForEachRefCmd(workingDir, args, &output))
        return;

    refs.clear();
    refs = output.split(QLatin1String("\n"));
    foreach (const QString &reference, refs) {
        QStringList entries = reference.split(QLatin1Char('\t'));
        if (entries.count() < 2 || entries.first().endsWith(head))
            continue;
        const QString ref = entries.at(0).mid(remotesPrefix.size());
        int refBranchIndex = ref.indexOf(QLatin1Char('/'));
        int timeT = entries.at(1).left(entries.at(1).indexOf(QLatin1Char(' '))).toInt();
        BranchDate bd(ref.mid(refBranchIndex + 1), QDateTime::fromTime_t(timeT).date());
        m_remoteBranches.insertMulti(ref.left(refBranchIndex), bd);
    }

    int currIndex = 0;
    QStringList remotes = m_remoteBranches.keys();
    remotes.removeDuplicates();
    foreach (const QString &remote, remotes) {
        m_ui->remoteComboBox->addItem(remote);
        if (remote == m_suggestedRemoteName)
            m_ui->remoteComboBox->setCurrentIndex(currIndex);
        ++currIndex;
    }
    const int remoteCount = m_ui->remoteComboBox->count();
    if (remoteCount < 1) {
        return;
    } else if (remoteCount == 1) {
        m_ui->remoteLabel->hide();
        m_ui->remoteComboBox->hide();
    } else {
        connect(m_ui->remoteComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setRemoteBranches()));
    }
    connect(m_ui->branchComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setChangeRange()));
    setRemoteBranches();
    m_ui->reviewersLineEdit->setText(reviewerList);

    m_ui->topicLineEdit->setValidator(new QRegExpValidator(QRegExp(QLatin1String("^\\S+$")), this));

    m_valid = true;
}

GerritPushDialog::~GerritPushDialog()
{
    delete m_ui;
}

QString GerritPushDialog::selectedCommit() const
{
    return m_ui->commitView->commit();
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
    if (m_ui->branchComboBox->itemData(m_ui->branchComboBox->currentIndex()) == 1) {
        setRemoteBranches(true);
        return;
    }
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

bool GerritPushDialog::valid() const
{
    return m_valid;
}

void GerritPushDialog::setRemoteBranches(bool includeOld)
{
    bool blocked = m_ui->branchComboBox->blockSignals(true);
    m_ui->branchComboBox->clear();

    int i = 0;
    bool excluded = false;
    for (RemoteBranchesMap::const_iterator it = m_remoteBranches.constBegin(),
         end = m_remoteBranches.constEnd();
         it != end; ++it) {
        if (it.key() == selectedRemoteName()) {
            const BranchDate &bd = it.value();
            const bool isSuggested = bd.first == m_suggestedRemoteBranch;
            if (includeOld || bd.second.daysTo(QDate::currentDate()) <= 60 || isSuggested) {
                m_ui->branchComboBox->addItem(bd.first);
                if (isSuggested)
                    m_ui->branchComboBox->setCurrentIndex(i);
                ++i;
            } else {
                excluded = true;
            }
        }
    }
    if (excluded)
        m_ui->branchComboBox->addItem(tr("... Include older branches ..."), 1);
    setChangeRange();
    m_ui->branchComboBox->blockSignals(blocked);
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
    return m_ui->draftCheckBox->isChecked() ? QLatin1String("drafts") : QLatin1String("for");
}

QString GerritPushDialog::selectedTopic() const
{
    return m_ui->topicLineEdit->text().trimmed();
}

QString GerritPushDialog::reviewers() const
{
    return m_ui->reviewersLineEdit->text();
}

} // namespace Internal
} // namespace Gerrit
