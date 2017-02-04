/****************************************************************************
**
** Copyright (C) 2016 Petar Perisin.
** Contact: petar.perisin@gmail.com
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

#include "gerritpushdialog.h"
#include "ui_gerritpushdialog.h"
#include "branchcombobox.h"

#include "../gitplugin.h"
#include "../gitclient.h"
#include "../gitconstants.h"

#include <utils/icon.h>

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
        : IconItemDelegate(widget, Utils::Icon(":/git/images/arrowup.png"))
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

    if (!GitPlugin::client()->synchronousBranchCmd(
                m_workingDir, { "-r", "--contains", earliestCommit + '^' }, &output, &error)) {
        return QString();
    }
    const QString head = "/HEAD";
    const QStringList refs = output.split('\n');

    QString remoteTrackingBranch;
    if (localBranch != "HEAD")
        remoteTrackingBranch = GitPlugin::client()->synchronousTrackingBranch(m_workingDir, localBranch);

    QString remoteBranch;
    for (const QString &reference : refs) {
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
    const QString head = "/HEAD";

    QString remotesPrefix("refs/remotes/");
    if (!GitPlugin::client()->synchronousForEachRefCmd(
                m_workingDir, { "--format=%(refname)\t%(committerdate:raw)", remotesPrefix }, &output)) {
        return;
    }

    const QStringList refs = output.split("\n");
    for (const QString &reference : refs) {
        QStringList entries = reference.split('\t');
        if (entries.count() < 2 || entries.first().endsWith(head))
            continue;
        const QString ref = entries.at(0).mid(remotesPrefix.size());
        int refBranchIndex = ref.indexOf('/');
        int timeT = entries.at(1).leftRef(entries.at(1).indexOf(' ')).toInt();
        BranchDate bd(ref.mid(refBranchIndex + 1), QDateTime::fromTime_t(timeT).date());
        m_remoteBranches.insertMulti(ref.left(refBranchIndex), bd);
    }
    QStringList remotes = GitPlugin::client()->synchronousRemotesList(m_workingDir).keys();
    remotes.removeDuplicates();
    {
        const QString origin = "origin";
        const QString gerrit = "gerrit";
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
    m_ui(new Ui::GerritPushDialog)
{
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

    connect(m_ui->targetBranchComboBox, &QComboBox::currentTextChanged,
            this, &GerritPushDialog::validate);

    updateCommits(m_ui->localBranchComboBox->currentIndex());
    setRemoteBranches();

    QRegExpValidator *noSpaceValidator = new QRegExpValidator(QRegExp("^\\S+$"), this);
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
    remote += '/';
    remote += selectedRemoteBranchName();

    QString number;
    QString error;

    GitPlugin::client()->synchronousRevListCmd(m_workingDir, { remote + ".." + branch, "--count" },
                                               &number, &error);

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
    const QString remote = selectedRemoteName() + '/' + remoteBranchName;
    m_ui->infoLabel->setText(
                tr("Number of commits between %1 and %2: %3").arg(branch, remote, range));
}

bool GerritPushDialog::isValid() const
{
    return m_isValid;
}

void GerritPushDialog::storeTopic()
{
    const QString branch = m_ui->localBranchComboBox->currentText();
    GitPlugin::client()->setConfigValue(m_workingDir, QString("branch.%1.topic").arg(branch),
                                        selectedTopic());
}

void GerritPushDialog::setRemoteBranches(bool includeOld)
{
    bool blocked = m_ui->targetBranchComboBox->blockSignals(true);
    m_ui->targetBranchComboBox->clear();

    const QString remoteName = selectedRemoteName();
    if (!m_remoteBranches.contains(remoteName)) {
        const QStringList remoteBranches =
                GitPlugin::client()->synchronousRepositoryBranches(remoteName, m_workingDir);
        for (const QString &branch : remoteBranches)
            m_remoteBranches.insertMulti(remoteName, qMakePair(branch, QDate()));
        if (remoteBranches.isEmpty()) {
            m_ui->targetBranchComboBox->setEditable(true);
            m_ui->targetBranchComboBox->setToolTip(
                        tr("No remote branches found. This is probably the initial commit."));
            if (QLineEdit *lineEdit = m_ui->targetBranchComboBox->lineEdit())
                lineEdit->setPlaceholderText(tr("Branch name"));
        }
    }

    int i = 0;
    bool excluded = false;
    const QList<BranchDate> remoteBranches = m_remoteBranches.values(remoteName);
    for (const BranchDate &bd : remoteBranches) {
        const bool isSuggested = bd.first == m_suggestedRemoteBranch;
        if (includeOld || isSuggested || !bd.second.isValid()
                || bd.second.daysTo(QDate::currentDate()) <= Git::Constants::OBSOLETE_COMMIT_AGE_IN_DAYS) {
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
    validate();
}

void GerritPushDialog::updateCommits(int index)
{
    const QString branch = m_ui->localBranchComboBox->itemText(index);
    m_hasLocalCommits = m_ui->commitView->init(m_workingDir, branch, LogChangeWidget::Silent);
    QString topic = GitPlugin::client()->readConfigValue(
                m_workingDir, QString("branch.%1.topic").arg(branch));
    if (!topic.isEmpty())
        m_ui->topicLineEdit->setText(topic);

    const QString remoteBranch = determineRemoteBranch(branch);
    if (!remoteBranch.isEmpty()) {
        const int slash = remoteBranch.indexOf('/');

        m_suggestedRemoteBranch = remoteBranch.mid(slash + 1);
        const QString remote = remoteBranch.left(slash);
        const int index = m_ui->remoteComboBox->findText(remote);
        if (index != -1 && index != m_ui->remoteComboBox->currentIndex())
            m_ui->remoteComboBox->setCurrentIndex(index);
        else
            setRemoteBranches();
    }
    validate();
}

void GerritPushDialog::validate()
{
    const bool valid = m_hasLocalCommits && !selectedRemoteBranchName().isEmpty();
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
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
    return QLatin1String(m_ui->draftCheckBox->isChecked() ? "drafts" : "for");
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
