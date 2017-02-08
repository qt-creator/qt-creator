/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "branchdialog.h"
#include "branchadddialog.h"
#include "branchcheckoutdialog.h"
#include "branchmodel.h"
#include "gitclient.h"
#include "gitplugin.h"
#include "gitutils.h"
#include "gitconstants.h"
#include "ui_branchdialog.h"

#include <vcsbase/vcsoutputwindow.h>
#include <coreplugin/documentmanager.h>

#include <utils/asconst.h>
#include <utils/execmenu.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QList>
#include <QMenu>

#include <QDebug>

using namespace VcsBase;

namespace Git {
namespace Internal {

BranchDialog::BranchDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::BranchDialog),
    m_model(new BranchModel(GitPlugin::client(), this))
{
    setModal(false);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setAttribute(Qt::WA_DeleteOnClose, true); // Do not update unnecessarily

    m_ui->setupUi(this);
    m_ui->includeOldCheckBox->setToolTip(
                tr("Include branches and tags that have not been active for %n days.", 0,
                   Constants::OBSOLETE_COMMIT_AGE_IN_DAYS));

    connect(m_ui->refreshButton, &QAbstractButton::clicked, this, &BranchDialog::refreshCurrentRepository);
    connect(m_ui->addButton, &QAbstractButton::clicked, this, &BranchDialog::add);
    connect(m_ui->checkoutButton, &QAbstractButton::clicked, this, &BranchDialog::checkout);
    connect(m_ui->removeButton, &QAbstractButton::clicked, this, &BranchDialog::remove);
    connect(m_ui->renameButton, &QAbstractButton::clicked, this, &BranchDialog::rename);
    connect(m_ui->diffButton, &QAbstractButton::clicked, this, &BranchDialog::diff);
    connect(m_ui->logButton, &QAbstractButton::clicked, this, &BranchDialog::log);
    connect(m_ui->resetButton, &QAbstractButton::clicked, this, &BranchDialog::reset);
    connect(m_ui->mergeButton, &QAbstractButton::clicked, this, &BranchDialog::merge);
    connect(m_ui->rebaseButton, &QAbstractButton::clicked, this, &BranchDialog::rebase);
    connect(m_ui->cherryPickButton, &QAbstractButton::clicked, this, &BranchDialog::cherryPick);
    connect(m_ui->trackButton, &QAbstractButton::clicked, this, &BranchDialog::setRemoteTracking);
    connect(m_ui->includeOldCheckBox, &QCheckBox::toggled, this, [this](bool value) {
        m_model->setOldBranchesIncluded(value);
        refreshCurrentRepository();
    });

    m_ui->branchView->setModel(m_model);
    m_ui->branchView->setFocus();

    connect(m_ui->branchView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &BranchDialog::enableButtons);
    connect(m_model, &QAbstractItemModel::dataChanged, this, &BranchDialog::resizeColumns);
    connect(m_model, &QAbstractItemModel::rowsInserted, this, &BranchDialog::resizeColumns);
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, &BranchDialog::resizeColumns);

    m_ui->branchView->selectionModel()->clear();
}

BranchDialog::~BranchDialog()
{
    delete m_ui;
}

void BranchDialog::refresh(const QString &repository, bool force)
{
    if (m_repository == repository && !force)
        return;

    m_repository = repository;
    m_ui->repositoryLabel->setText(GitPlugin::msgRepositoryLabel(m_repository));
    QString errorMessage;
    if (!m_model->refresh(m_repository, &errorMessage))
        VcsOutputWindow::appendError(errorMessage);

    m_ui->branchView->expandAll();
    resizeColumns();
}

void BranchDialog::refreshIfSame(const QString &repository)
{
    if (m_repository == repository)
        refreshCurrentRepository();
}

void BranchDialog::resizeColumns()
{
    m_ui->branchView->resizeColumnToContents(0);
    m_ui->branchView->resizeColumnToContents(1);
}

void BranchDialog::enableButtons()
{
    QModelIndex idx = selectedIndex();
    QModelIndex currentBranch = m_model->currentBranch();
    const bool hasSelection = idx.isValid();
    const bool currentSelected = hasSelection && idx == currentBranch;
    const bool isLocal = m_model->isLocal(idx);
    const bool isLeaf = m_model->isLeaf(idx);
    const bool isTag = m_model->isTag(idx);
    const bool hasActions = hasSelection && isLeaf;
    const bool currentLocal = m_model->isLocal(currentBranch);

    m_ui->removeButton->setEnabled(hasActions && !currentSelected && (isLocal || isTag));
    m_ui->renameButton->setEnabled(hasActions && (isLocal || isTag));
    m_ui->logButton->setEnabled(hasActions);
    m_ui->diffButton->setEnabled(hasActions);
    m_ui->checkoutButton->setEnabled(hasActions && !currentSelected);
    m_ui->rebaseButton->setEnabled(hasActions && !currentSelected);
    m_ui->resetButton->setEnabled(hasActions && currentLocal && !currentSelected);
    m_ui->mergeButton->setEnabled(hasActions && !currentSelected);
    m_ui->cherryPickButton->setEnabled(hasActions && !currentSelected);
    m_ui->trackButton->setEnabled(hasActions && currentLocal && !currentSelected && !isTag);
}

void BranchDialog::refreshCurrentRepository()
{
    refresh(m_repository, true);
}

void BranchDialog::add()
{
    QModelIndex trackedIndex = selectedIndex();
    QString trackedBranch = m_model->fullName(trackedIndex);
    if (trackedBranch.isEmpty()) {
        trackedIndex = m_model->currentBranch();
        trackedBranch = m_model->fullName(trackedIndex);
    }
    const bool isLocal = m_model->isLocal(trackedIndex);
    const bool isTag = m_model->isTag(trackedIndex);

    QStringList localNames = m_model->localBranchNames();

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

    BranchAddDialog branchAddDialog(localNames, true, this);
    branchAddDialog.setBranchName(suggestedName);
    branchAddDialog.setTrackedBranchName(isTag ? QString() : trackedBranch, !isLocal);

    if (branchAddDialog.exec() == QDialog::Accepted) {
        QModelIndex idx = m_model->addBranch(branchAddDialog.branchName(), branchAddDialog.track(), trackedIndex);
        if (!idx.isValid())
            return;
        m_ui->branchView->selectionModel()->select(idx, QItemSelectionModel::Clear
                                                        | QItemSelectionModel::Select
                                                        | QItemSelectionModel::Current);
        m_ui->branchView->scrollTo(idx);
        if (QMessageBox::question(this, tr("Checkout"), tr("Checkout branch?"),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            checkout();
    }
}

void BranchDialog::checkout()
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return;
    QModelIndex idx = selectedIndex();

    const QString currentBranch = m_model->fullName(m_model->currentBranch());
    const QString nextBranch = m_model->fullName(idx);
    const QString popMessageStart = QCoreApplication::applicationName() +
            ' ' + nextBranch + "-AutoStash ";

    BranchCheckoutDialog branchCheckoutDialog(this, currentBranch, nextBranch);
    GitClient *client = GitPlugin::client();

    if (client->gitStatus(m_repository, StatusMode(NoUntracked | NoSubmodules)) != GitClient::StatusChanged)
        branchCheckoutDialog.foundNoLocalChanges();

    QList<Stash> stashes;
    client->synchronousStashList(m_repository, &stashes);
    for (const Stash &stash : Utils::asConst(stashes)) {
        if (stash.message.startsWith(popMessageStart)) {
            branchCheckoutDialog.foundStashForNextBranch();
            break;
        }
    }

    if (!branchCheckoutDialog.hasLocalChanges() &&
        !branchCheckoutDialog.hasStashForNextBranch()) {
        // No local changes and no Auto Stash - no need to open dialog
        m_model->checkoutBranch(idx);
    } else if (branchCheckoutDialog.exec() == QDialog::Accepted) {

        if (branchCheckoutDialog.makeStashOfCurrentBranch()) {
            if (client->synchronousStash(m_repository, currentBranch + "-AutoStash").isEmpty())
                return;
        } else if (branchCheckoutDialog.moveLocalChangesToNextBranch()) {
            if (!client->beginStashScope(m_repository, "Checkout", NoPrompt))
                return;
        } else if (branchCheckoutDialog.discardLocalChanges()) {
            if (!client->synchronousReset(m_repository))
                return;
        }

        m_model->checkoutBranch(idx);

        QString stashName;
        client->synchronousStashList(m_repository, &stashes);
        for (const Stash &stash : Utils::asConst(stashes)) {
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
    m_ui->branchView->selectionModel()->clear();
}

/* Prompt to delete a local branch and do so. */
void BranchDialog::remove()
{
    QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch()); // otherwise the button would not be enabled!

    QString branchName = m_model->fullName(selected);
    if (branchName.isEmpty())
        return;

    const bool isTag = m_model->isTag(selected);
    const bool wasMerged = isTag ? true : m_model->branchIsMerged(selected);
    QString message;
    if (isTag)
        message = tr("Would you like to delete the tag \"%1\"?").arg(branchName);
    else if (wasMerged)
        message = tr("Would you like to delete the branch \"%1\"?").arg(branchName);
    else
        message = tr("Would you like to delete the <b>unmerged</b> branch \"%1\"?").arg(branchName);

    if (QMessageBox::question(this, isTag ? tr("Delete Tag") : tr("Delete Branch"),
                              message, QMessageBox::Yes | QMessageBox::No,
                              wasMerged ? QMessageBox::Yes : QMessageBox::No) == QMessageBox::Yes) {
        if (isTag)
            m_model->removeTag(selected);
        else
            m_model->removeBranch(selected);
    }
}

void BranchDialog::rename()
{
    QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch()); // otherwise the button would not be enabled!
    const bool isTag = m_model->isTag(selected);
    QTC_CHECK(m_model->isLocal(selected) || isTag);

    QString oldName = m_model->fullName(selected);
    QStringList localNames;
    if (!isTag)
        localNames = m_model->localBranchNames();

    BranchAddDialog branchAddDialog(localNames, false, this);
    if (isTag)
        branchAddDialog.setWindowTitle(tr("Rename Tag"));
    branchAddDialog.setBranchName(oldName);
    branchAddDialog.setTrackedBranchName(QString(), false);

    branchAddDialog.exec();

    if (branchAddDialog.result() == QDialog::Accepted) {
        if (branchAddDialog.branchName() == oldName)
            return;
        if (isTag)
            m_model->renameTag(oldName, branchAddDialog.branchName());
        else
            m_model->renameBranch(oldName, branchAddDialog.branchName());
        refreshCurrentRepository();
    }
    m_ui->branchView->selectionModel()->clear();
}

void BranchDialog::diff()
{
    QString fullName = m_model->fullName(selectedIndex(), true);
    if (fullName.isEmpty())
        return;
    GitPlugin::client()->diffBranch(m_repository, fullName);
}

void BranchDialog::log()
{
    QString branchName = m_model->fullName(selectedIndex(), true);
    if (branchName.isEmpty())
        return;
    GitPlugin::client()->log(m_repository, QString(), false, { branchName });
}

void BranchDialog::reset()
{
    QString currentName = m_model->fullName(m_model->currentBranch());
    QString branchName = m_model->fullName(selectedIndex());
    if (currentName.isEmpty() || branchName.isEmpty())
        return;

    if (QMessageBox::question(this, tr("Git Reset"), tr("Hard reset branch \"%1\" to \"%2\"?")
                              .arg(currentName).arg(branchName),
                              QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        GitPlugin::client()->reset(m_repository, "--hard", branchName);
    }
}

void BranchDialog::merge()
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return;
    QModelIndex idx = selectedIndex();
    QTC_CHECK(idx != m_model->currentBranch()); // otherwise the button would not be enabled!

    const QString branch = m_model->fullName(idx, true);
    GitClient *client = GitPlugin::client();
    bool allowFastForward = true;
    if (client->isFastForwardMerge(m_repository, branch)) {
        QMenu popup;
        QAction *fastForward = popup.addAction(tr("Fast-Forward"));
        popup.addAction(tr("No Fast-Forward"));
        QAction *chosen = Utils::execMenuAtWidget(&popup, m_ui->mergeButton);
        if (!chosen)
            return;
        allowFastForward = (chosen == fastForward);
    }
    if (client->beginStashScope(m_repository, "merge", AllowUnstashed))
        client->synchronousMerge(m_repository, branch, allowFastForward);
}

void BranchDialog::rebase()
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return;
    QModelIndex idx = selectedIndex();
    QTC_CHECK(idx != m_model->currentBranch()); // otherwise the button would not be enabled!

    const QString baseBranch = m_model->fullName(idx, true);
    GitClient *client = GitPlugin::client();
    if (client->beginStashScope(m_repository, "rebase"))
        client->rebase(m_repository, baseBranch);
}

void BranchDialog::cherryPick()
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return;
    QModelIndex idx = selectedIndex();
    QTC_CHECK(idx != m_model->currentBranch()); // otherwise the button would not be enabled!

    const QString branch = m_model->fullName(idx, true);
    GitPlugin::client()->synchronousCherryPick(m_repository, branch);
}

void BranchDialog::setRemoteTracking()
{
    m_model->setRemoteTracking(selectedIndex());
}

QModelIndex BranchDialog::selectedIndex()
{
    QModelIndexList selected = m_ui->branchView->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
        return QModelIndex();
    return selected.at(0);
}

} // namespace Internal
} // namespace Git
