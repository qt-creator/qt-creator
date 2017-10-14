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
#include <QVersionNumber>

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
                m_workingDir, {"-r", "--contains", earliestCommit + '^'}, &output, &error)) {
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
                m_workingDir, {"--format=%(refname)\t%(committerdate:raw)", remotesPrefix}, &output)) {
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
    m_ui->remoteComboBox->updateRemotes(false);
}

GerritPushDialog::GerritPushDialog(const QString &workingDir, const QString &reviewerList,
                                   QSharedPointer<GerritParameters> parameters, QWidget *parent) :
    QDialog(parent),
    m_workingDir(workingDir),
    m_ui(new Ui::GerritPushDialog)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->repositoryLabel->setText(QDir::toNativeSeparators(workingDir));
    m_ui->remoteComboBox->setRepository(workingDir);
    m_ui->remoteComboBox->setParameters(parameters);
    m_ui->remoteComboBox->setAllowDups(true);

    PushItemDelegate *delegate = new PushItemDelegate(m_ui->commitView);
    delegate->setParent(this);

    initRemoteBranches();

    if (m_ui->remoteComboBox->isEmpty()) {
        m_initErrorMessage = tr("Cannot find a Gerrit remote. Add one and try again.");
        return;
    }

    m_ui->localBranchComboBox->init(workingDir);
    connect(m_ui->localBranchComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &GerritPushDialog::updateCommits);

    connect(m_ui->targetBranchComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &GerritPushDialog::setChangeRange);

    connect(m_ui->targetBranchComboBox, &QComboBox::currentTextChanged,
            this, &GerritPushDialog::validate);

    updateCommits(m_ui->localBranchComboBox->currentIndex());
    onRemoteChanged(true);

    QRegExpValidator *noSpaceValidator = new QRegExpValidator(QRegExp("^\\S+$"), this);
    m_ui->reviewersLineEdit->setText(reviewerList);
    m_ui->reviewersLineEdit->setValidator(noSpaceValidator);
    m_ui->topicLineEdit->setValidator(noSpaceValidator);
    m_ui->wipCheckBox->setCheckState(Qt::PartiallyChecked);

    connect(m_ui->remoteComboBox, &GerritRemoteChooser::remoteChanged,
            this, [this] { onRemoteChanged(); });
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

static bool versionSupportsWip(const QString &version)
{
    return QVersionNumber::fromString(version) >= QVersionNumber(2, 15);
}

void GerritPushDialog::onRemoteChanged(bool force)
{
    setRemoteBranches();
    const QString version = m_ui->remoteComboBox->currentServer().version;
    const bool supportsWip = versionSupportsWip(version);
    if (!force && supportsWip == m_currentSupportsWip)
        return;
    m_currentSupportsWip = supportsWip;
    m_ui->wipCheckBox->setEnabled(supportsWip);
    if (supportsWip) {
        m_ui->wipCheckBox->setToolTip(tr("Checked - Mark change as WIP\n"
                                         "Unchecked - Mark change as ready\n"
                                         "Partially checked - Do not change current state"));
        m_ui->draftCheckBox->setTristate(true);
        if (m_ui->draftCheckBox->checkState() != Qt::Checked)
            m_ui->draftCheckBox->setCheckState(Qt::PartiallyChecked);
        m_ui->draftCheckBox->setToolTip(tr("Checked - Mark change as private\n"
                                           "Unchecked - Unmark change as private\n"
                                           "Partially checked - Do not change current state"));
    } else {
        m_ui->wipCheckBox->setToolTip(tr("Supported on Gerrit 2.15 and up"));
        m_ui->draftCheckBox->setTristate(false);
        if (m_ui->draftCheckBox->checkState() != Qt::Checked)
            m_ui->draftCheckBox->setCheckState(Qt::Unchecked);
        m_ui->draftCheckBox->setToolTip(tr("Checked - Mark change as draft\n"
                                           "Unchecked - Unmark change as draft"));
    }
}

QString GerritPushDialog::initErrorMessage() const
{
    return m_initErrorMessage;
}

QString GerritPushDialog::pushTarget() const
{
    QStringList options;
    QString target = selectedCommit();
    if (target.isEmpty())
        target = "HEAD";
    target += ":refs/";
    if (versionSupportsWip(m_ui->remoteComboBox->currentServer().version)) {
        target += "for";
        const Qt::CheckState draftState = m_ui->draftCheckBox->checkState();
        const Qt::CheckState wipState = m_ui->wipCheckBox->checkState();
        if (draftState == Qt::Checked)
            options << "private";
        else if (draftState == Qt::Unchecked)
            options << "remove-private";

        if (wipState == Qt::Checked)
            options << "wip";
        else if (wipState == Qt::Unchecked)
            options << "ready";
    } else {
        target += QLatin1String(m_ui->draftCheckBox->isChecked() ? "for" : "drafts");
    }
    target += '/' + selectedRemoteBranchName();
    const QString topic = selectedTopic();
    if (!topic.isEmpty())
        target += '/' + topic;

    const QStringList reviewersInput = reviewers().split(',', QString::SkipEmptyParts);
    for (const QString &reviewer : reviewersInput)
        options << "r=" + reviewer;

    if (!options.isEmpty())
        target += '%' + options.join(',');
    return target;
}

void GerritPushDialog::storeTopic()
{
    const QString branch = m_ui->localBranchComboBox->currentText();
    GitPlugin::client()->setConfigValue(m_workingDir, QString("branch.%1.topic").arg(branch),
                                        selectedTopic());
}

void GerritPushDialog::setRemoteBranches(bool includeOld)
{
    {
        QSignalBlocker blocker(m_ui->targetBranchComboBox);
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
    }
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

        if (!m_ui->remoteComboBox->setCurrentRemote(remote))
            onRemoteChanged();
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
    return m_ui->remoteComboBox->currentRemoteName();
}

QString GerritPushDialog::selectedRemoteBranchName() const
{
    return m_ui->targetBranchComboBox->currentText();
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
