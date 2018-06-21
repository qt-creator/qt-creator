/****************************************************************************
**
** Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
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

#include "branchutils.h"

#include "branchadddialog.h"
#include "branchcheckoutdialog.h"
#include "branchmodel.h"
#include "gitclient.h"
#include "gitplugin.h"
#include "gitutils.h"

#include <coreplugin/documentmanager.h>
#include <utils/qtcassert.h>

#include <QMessageBox>
#include <QTreeView>

namespace Git {
namespace Internal {

BranchUtils::BranchUtils(QWidget *parent)
    : m_model(new BranchModel(GitPlugin::client(), parent))
    , m_widget(parent)
{
}

void BranchUtils::setBranchView(QTreeView *branchView)
{
    m_branchView = branchView;
}

QModelIndex BranchUtils::selectedIndex()
{
    QTC_ASSERT(m_branchView, return QModelIndex());
    QModelIndexList selected = m_branchView->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
        return QModelIndex();
    return selected.at(0);
}

bool BranchUtils::add()
{
    QModelIndex trackedIndex = selectedIndex();
    QString trackedBranch = m_model->fullName(trackedIndex);
    if (trackedBranch.isEmpty()) {
        trackedIndex = m_model->currentBranch();
        trackedBranch = m_model->fullName(trackedIndex);
    }
    const bool isLocal = m_model->isLocal(trackedIndex);
    const bool isTag = m_model->isTag(trackedIndex);

    const QStringList localNames = m_model->localBranchNames();

    QString suggestedName;
    if (!isTag) {
        const QString suggestedNameBase = trackedBranch.mid(trackedBranch.lastIndexOf('/') + 1);
        suggestedName = suggestedNameBase;
        int i = 2;
        while (localNames.contains(suggestedName)) {
            suggestedName = suggestedNameBase + QString::number(i);
            ++i;
        }
    }

    BranchAddDialog branchAddDialog(localNames, true, m_widget);
    branchAddDialog.setBranchName(suggestedName);
    branchAddDialog.setTrackedBranchName(isTag ? QString() : trackedBranch, !isLocal);

    if (branchAddDialog.exec() == QDialog::Accepted) {
        QModelIndex idx = m_model->addBranch(branchAddDialog.branchName(), branchAddDialog.track(), trackedIndex);
        if (!idx.isValid())
            return false;
        QTC_ASSERT(m_branchView, return false);
        m_branchView->selectionModel()->select(idx, QItemSelectionModel::Clear
                                             | QItemSelectionModel::Select
                                             | QItemSelectionModel::Current);
        m_branchView->scrollTo(idx);
        if (QMessageBox::question(m_widget, tr("Checkout"), tr("Checkout branch?"),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            return checkout();
        }
    }

    return false;
}

bool BranchUtils::checkout()
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return false;

    const QModelIndex selected = selectedIndex();
    const QString currentBranch = m_model->fullName(m_model->currentBranch());
    const QString nextBranch = m_model->fullName(selected);
    const QString popMessageStart = QCoreApplication::applicationName() +
            ' ' + nextBranch + "-AutoStash ";

    BranchCheckoutDialog branchCheckoutDialog(m_widget, currentBranch, nextBranch);
    GitClient *client = GitPlugin::client();

    if (client->gitStatus(m_repository, StatusMode(NoUntracked | NoSubmodules)) != GitClient::StatusChanged)
        branchCheckoutDialog.foundNoLocalChanges();

    QList<Stash> stashes;
    client->synchronousStashList(m_repository, &stashes);
    for (const Stash &stash : qAsConst(stashes)) {
        if (stash.message.startsWith(popMessageStart)) {
            branchCheckoutDialog.foundStashForNextBranch();
            break;
        }
    }

    if (!branchCheckoutDialog.hasLocalChanges() &&
        !branchCheckoutDialog.hasStashForNextBranch()) {
        // No local changes and no Auto Stash - no need to open dialog
        m_model->checkoutBranch(selected);
    } else if (branchCheckoutDialog.exec() == QDialog::Accepted) {

        if (branchCheckoutDialog.makeStashOfCurrentBranch()) {
            if (client->synchronousStash(m_repository, currentBranch + "-AutoStash").isEmpty())
                return false;
        } else if (branchCheckoutDialog.moveLocalChangesToNextBranch()) {
            if (!client->beginStashScope(m_repository, "Checkout", NoPrompt))
                return false;
        } else if (branchCheckoutDialog.discardLocalChanges()) {
            if (!client->synchronousReset(m_repository))
                return false;
        }

        m_model->checkoutBranch(selected);

        QString stashName;
        client->synchronousStashList(m_repository, &stashes);
        for (const Stash &stash : qAsConst(stashes)) {
            if (stash.message.startsWith(popMessageStart)) {
                stashName = stash.name;
                break;
            }
        }

        if (branchCheckoutDialog.moveLocalChangesToNextBranch())
            client->endStashScope(m_repository);
        else if (branchCheckoutDialog.popStashOfNextBranch())
            client->synchronousStashRestore(m_repository, stashName, true);
    }

    if (QTC_GUARD(m_branchView))
        m_branchView->selectionModel()->clear();
    return true;
}

// Prompt to delete a local branch and do so.
bool BranchUtils::remove()
{
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    QString branchName = m_model->fullName(selected);
    if (branchName.isEmpty())
        return false;

    const bool isTag = m_model->isTag(selected);
    const bool wasMerged = isTag ? true : m_model->branchIsMerged(selected);
    QString message;
    if (isTag)
        message = tr("Would you like to delete the tag \"%1\"?").arg(branchName);
    else if (wasMerged)
        message = tr("Would you like to delete the branch \"%1\"?").arg(branchName);
    else
        message = tr("Would you like to delete the <b>unmerged</b> branch \"%1\"?").arg(branchName);

    if (QMessageBox::question(m_widget, isTag ? tr("Delete Tag") : tr("Delete Branch"),
                              message, QMessageBox::Yes | QMessageBox::No,
                              wasMerged ? QMessageBox::Yes : QMessageBox::No) == QMessageBox::Yes) {
        if (isTag)
            m_model->removeTag(selected);
        else
            m_model->removeBranch(selected);
    }

    return true;
}

bool BranchUtils::rename()
{
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());
    const bool isTag = m_model->isTag(selected);
    QTC_CHECK(m_model->isLocal(selected) || isTag);

    QString oldName = m_model->fullName(selected);
    QStringList localNames;
    if (!isTag)
        localNames = m_model->localBranchNames();

    BranchAddDialog branchAddDialog(localNames, false, m_widget);
    if (isTag)
        branchAddDialog.setWindowTitle(tr("Rename Tag"));
    branchAddDialog.setBranchName(oldName);
    branchAddDialog.setTrackedBranchName(QString(), false);

    branchAddDialog.exec();

    if (branchAddDialog.result() == QDialog::Accepted) {
        if (branchAddDialog.branchName() == oldName)
            return false;
        if (isTag)
            m_model->renameTag(oldName, branchAddDialog.branchName());
        else
            m_model->renameBranch(oldName, branchAddDialog.branchName());
        return true;
    }

    if (QTC_GUARD(m_branchView))
        m_branchView->selectionModel()->clear();
    return false;
}

bool BranchUtils::reset()
{
    const QString currentName = m_model->fullName(m_model->currentBranch());
    const QString branchName = m_model->fullName(selectedIndex());
    if (currentName.isEmpty() || branchName.isEmpty())
        return false;

    if (QMessageBox::question(m_widget, tr("Git Reset"), tr("Hard reset branch \"%1\" to \"%2\"?")
                              .arg(currentName).arg(branchName),
                              QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        GitPlugin::client()->reset(m_repository, "--hard", branchName);
        return true;
    }
    return false;
}

bool BranchUtils::isFastForwardMerge()
{
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    const QString branch = m_model->fullName(selected, true);
    return GitPlugin::client()->isFastForwardMerge(m_repository, branch);
}

bool BranchUtils::merge(bool allowFastForward)
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return false;
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    const QString branch = m_model->fullName(selected, true);
    GitClient *client = GitPlugin::client();
    if (client->beginStashScope(m_repository, "merge", AllowUnstashed))
        return client->synchronousMerge(m_repository, branch, allowFastForward);

    return false;
}

void BranchUtils::rebase()
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return;
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    const QString baseBranch = m_model->fullName(selected, true);
    GitClient *client = GitPlugin::client();
    if (client->beginStashScope(m_repository, "rebase"))
        client->rebase(m_repository, baseBranch);
}

bool BranchUtils::cherryPick()
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return false;
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    const QString branch = m_model->fullName(selected, true);
    return GitPlugin::client()->synchronousCherryPick(m_repository, branch);
}

} // namespace Internal
} // namespace Git
