/****************************************************************************
**
** Copyright (C) 2015 Petar Perisin.
** Contact: petar.perisin@gmail.com
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gerritpushdialog.h"
#include "ui_gerritpushdialog.h"
#include "branchcombobox.h"

#include "../gitplugin.h"
#include "../gitclient.h"

#include <QDateTime>
#include <QDir>
#include <QPushButton>
#include <QRegExpValidator>

using namespace Git::Internal;

namespace Gerrit {
namespace Internal {

class PushItemDelegate : public IconItemDelegate
{
public:
    PushItemDelegate(LogChangeWidget *widget)
        : IconItemDelegate(widget, QLatin1String(":/git/images/arrowup.png"))
    {
    }

protected:
    bool hasIcon(int row) const override
    {
        return row >= currentRow();
    }
};

QString GerritPushDialog::determineRemoteBranch(const QString &localBranch)
{
    const QString earliestCommit = m_ui->commitView->earliestCommit();

    QString output;
    QString error;
    QStringList args;
    args << QLatin1String("-r") << QLatin1String("--contains")
         << earliestCommit + QLatin1Char('^');

    if (!m_client->synchronousBranchCmd(m_workingDir, args, &output, &error))
        return QString();
    const QString head = QLatin1String("/HEAD");
    QStringList refs = output.split(QLatin1Char('\n'));

    QString remoteTrackingBranch;
    if (localBranch != QLatin1String("HEAD"))
        remoteTrackingBranch = m_client->synchronousTrackingBranch(m_workingDir, localBranch);

    QString remoteBranch;
    foreach (const QString &reference, refs) {
        const QString ref = reference.trimmed();
        if (ref.contains(head) || ref.isEmpty())
            continue;

        if (remoteBranch.isEmpty())
            remoteBranch = ref;

        // Prefer remote tracking branch if it exists and contains the latest remote commit
        if (ref == remoteTrackingBranch)
            return ref;
    }
    return remoteBranch;
}

void GerritPushDialog::initRemoteBranches()
{
    QString output;
    QStringList args;
    const QString head = QLatin1String("/HEAD");

    QString remotesPrefix(QLatin1String("refs/remotes/"));
    args << QLatin1String("--format=%(refname)\t%(committerdate:raw)")
         << remotesPrefix;
    if (!m_client->synchronousForEachRefCmd(m_workingDir, args, &output))
        return;

    const QStringList refs = output.split(QLatin1String("\n"));
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
    QStringList remotes = m_client->synchronousRemotesList(m_workingDir).keys();
    remotes.removeDuplicates();
    {
        const QString origin = QLatin1String("origin");
        const QString gerrit = QLatin1String("gerrit");
        if (remotes.removeOne(origin))
            remotes.prepend(origin);
        if (remotes.removeOne(gerrit))
            remotes.prepend(gerrit);
    }
    m_ui->remoteComboBox->addItems(remotes);
    m_ui->remoteComboBox->setEnabled(remotes.count() > 1);
}

GerritPushDialog::GerritPushDialog(const QString &workingDir, const QString &reviewerList, QWidget *parent) :
    QDialog(parent),
    m_workingDir(workingDir),
    m_ui(new Ui::GerritPushDialog),
    m_isValid(false)
{
    m_client = GitPlugin::instance()->client();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->repositoryLabel->setText(QDir::toNativeSeparators(workingDir));

    PushItemDelegate *delegate = new PushItemDelegate(m_ui->commitView);
    delegate->setParent(this);

    initRemoteBranches();

    if (m_ui->remoteComboBox->count() < 1)
        return;

    m_ui->localBranchComboBox->init(workingDir);
    connect(m_ui->localBranchComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &GerritPushDialog::updateCommits);

    connect(m_ui->targetBranchComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &GerritPushDialog::setChangeRange);

    updateCommits(m_ui->localBranchComboBox->currentIndex());
    setRemoteBranches();

    QRegExpValidator *noSpaceValidator = new QRegExpValidator(QRegExp(QLatin1String("^\\S+$")), this);
    m_ui->reviewersLineEdit->setText(reviewerList);
    m_ui->reviewersLineEdit->setValidator(noSpaceValidator);
    m_ui->topicLineEdit->setValidator(noSpaceValidator);

    connect(m_ui->remoteComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &GerritPushDialog::setRemoteBranches);

    m_isValid = true;
}

GerritPushDialog::~GerritPushDialog()
{
    delete m_ui;
}

QString GerritPushDialog::selectedCommit() const
{
    return m_ui->commitView->commit();
}

QString GerritPushDialog::calculateChangeRange(const QString &branch)
{
    QString remote = selectedRemoteName();
    remote += QLatin1Char('/');
    remote += selectedRemoteBranchName();

    QStringList args(remote + QLatin1String("..") + branch);
    args << QLatin1String("--count");

    QString number;
    QString error;

    m_client->synchronousRevListCmd(m_workingDir, args, &number, &error);

    number.chop(1);
    return number;
}

void GerritPushDialog::setChangeRange()
{
    if (m_ui->targetBranchComboBox->itemData(m_ui->targetBranchComboBox->currentIndex()) == 1) {
        setRemoteBranches(true);
        return;
    }
    const QString remoteBranchName = selectedRemoteBranchName();
    if (remoteBranchName.isEmpty())
        return;
    const QString branch = m_ui->localBranchComboBox->currentText();
    const QString range = calculateChangeRange(branch);
    if (range.isEmpty()) {
        m_ui->infoLabel->hide();
        return;
    }
    m_ui->infoLabel->show();
    const QString remote = selectedRemoteName() + QLatin1Char('/') + remoteBranchName;
    m_ui->infoLabel->setText(
                tr("Number of commits between %1 and %2: %3").arg(branch, remote, range));
}

bool GerritPushDialog::isValid() const
{
    return m_isValid;
}

void GerritPushDialog::setRemoteBranches(bool includeOld)
{
    bool blocked = m_ui->targetBranchComboBox->blockSignals(true);
    m_ui->targetBranchComboBox->clear();

    const QString remoteName = selectedRemoteName();
    if (!m_remoteBranches.contains(remoteName)) {
        foreach (const QString &branch, m_client->synchronousRepositoryBranches(remoteName, m_workingDir))
            m_remoteBranches.insertMulti(remoteName, qMakePair(branch, QDate()));
    }

    int i = 0;
    bool excluded = false;
    foreach (const BranchDate &bd, m_remoteBranches.values(remoteName)) {
        const bool isSuggested = bd.first == m_suggestedRemoteBranch;
        if (includeOld || isSuggested || !bd.second.isValid()
                || bd.second.daysTo(QDate::currentDate()) <= 60) {
            m_ui->targetBranchComboBox->addItem(bd.first);
            if (isSuggested)
                m_ui->targetBranchComboBox->setCurrentIndex(i);
            ++i;
        } else {
            excluded = true;
        }
    }
    if (excluded)
        m_ui->targetBranchComboBox->addItem(tr("... Include older branches ..."), 1);
    setChangeRange();
    m_ui->targetBranchComboBox->blockSignals(blocked);
}

void GerritPushDialog::updateCommits(int index)
{
    const QString branch = m_ui->localBranchComboBox->itemText(index);
    const bool hasLocalCommits = m_ui->commitView->init(m_workingDir, branch,
                                                        LogChangeWidget::Silent);

    const QString remoteBranch = determineRemoteBranch(branch);
    if (!remoteBranch.isEmpty()) {
        const int slash = remoteBranch.indexOf(QLatin1Char('/'));

        m_suggestedRemoteBranch = remoteBranch.mid(slash + 1);
        const QString remote = remoteBranch.left(slash);
        const int index = m_ui->remoteComboBox->findText(remote);
        if (index != -1 && index != m_ui->remoteComboBox->currentIndex())
            m_ui->remoteComboBox->setCurrentIndex(index);
        else
            setRemoteBranches();
    }

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(hasLocalCommits);
}

QString GerritPushDialog::selectedRemoteName() const
{
    return m_ui->remoteComboBox->currentText();
}

QString GerritPushDialog::selectedRemoteBranchName() const
{
    return m_ui->targetBranchComboBox->currentText();
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
