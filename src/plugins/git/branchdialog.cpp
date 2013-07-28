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

#include "branchdialog.h"
#include "branchadddialog.h"
#include "branchcheckoutdialog.h"
#include "branchmodel.h"
#include "gitclient.h"
#include "gitplugin.h"
#include "gitutils.h"
#include "ui_branchdialog.h"
#include "stashdialog.h" // Label helpers

#include <utils/qtcassert.h>
#include <vcsbase/vcsbaseoutputwindow.h>

#include <QItemSelectionModel>
#include <QMessageBox>
#include <QList>

#include <QDebug>

namespace Git {
namespace Internal {

BranchDialog::BranchDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::BranchDialog),
    m_model(new BranchModel(GitPlugin::instance()->gitClient(), this))
{
    setModal(false);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setAttribute(Qt::WA_DeleteOnClose, true); // Do not update unnecessarily

    m_ui->setupUi(this);

    connect(m_ui->refreshButton, SIGNAL(clicked()), this, SLOT(refresh()));
    connect(m_ui->addButton, SIGNAL(clicked()), this, SLOT(add()));
    connect(m_ui->checkoutButton, SIGNAL(clicked()), this, SLOT(checkout()));
    connect(m_ui->removeButton, SIGNAL(clicked()), this, SLOT(remove()));
    connect(m_ui->renameButton, SIGNAL(clicked()), this, SLOT(rename()));
    connect(m_ui->diffButton, SIGNAL(clicked()), this, SLOT(diff()));
    connect(m_ui->logButton, SIGNAL(clicked()), this, SLOT(log()));
    connect(m_ui->mergeButton, SIGNAL(clicked()), this, SLOT(merge()));
    connect(m_ui->rebaseButton, SIGNAL(clicked()), this, SLOT(rebase()));

    m_ui->branchView->setModel(m_model);

    connect(m_ui->branchView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(enableButtons()));

    enableButtons();
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
    m_ui->repositoryLabel->setText(StashDialog::msgRepositoryLabel(m_repository));
    QString errorMessage;
    if (!m_model->refresh(m_repository, &errorMessage))
        VcsBase::VcsBaseOutputWindow::instance()->appendError(errorMessage);

    m_ui->branchView->expandAll();
}

void BranchDialog::refreshIfSame(const QString &repository)
{
    if (m_repository == repository)
        refresh();
}

void BranchDialog::enableButtons()
{
    QModelIndex idx = selectedIndex();
    const bool hasSelection = idx.isValid();
    const bool currentSelected = hasSelection && idx == m_model->currentBranch();
    const bool isLocal = m_model->isLocal(idx);
    const bool isLeaf = m_model->isLeaf(idx);

    m_ui->removeButton->setEnabled(hasSelection && !currentSelected && isLocal && isLeaf);
    m_ui->renameButton->setEnabled(hasSelection && isLocal && isLeaf);
    m_ui->logButton->setEnabled(hasSelection && isLeaf);
    m_ui->diffButton->setEnabled(hasSelection && isLeaf);
    m_ui->checkoutButton->setEnabled(hasSelection && !currentSelected && isLeaf);
    m_ui->rebaseButton->setEnabled(hasSelection && !currentSelected && isLeaf);
    m_ui->mergeButton->setEnabled(hasSelection && !currentSelected && isLeaf);
}

void BranchDialog::refresh()
{
    refresh(m_repository, true);
}

void BranchDialog::add()
{
    QModelIndex trackedIndex = selectedIndex();
    QString trackedBranch = m_model->branchName(trackedIndex);
    if (trackedBranch.isEmpty()) {
        trackedIndex = m_model->currentBranch();
        trackedBranch = m_model->branchName(trackedIndex);
    }
    const bool isLocal = m_model->isLocal(trackedIndex);
    const bool isTag = m_model->isTag(trackedIndex);

    QStringList localNames = m_model->localBranchNames();

    QString suggestedNameBase = trackedBranch.mid(trackedBranch.lastIndexOf(QLatin1Char('/')) + 1);
    QString suggestedName = suggestedNameBase;
    int i = 2;
    while (localNames.contains(suggestedName)) {
        suggestedName = suggestedNameBase + QString::number(i);
        ++i;
    }

    BranchAddDialog branchAddDialog(true, this);
    branchAddDialog.setBranchName(suggestedName);
    branchAddDialog.setTrackedBranchName(isTag ? QString() : trackedBranch, !isLocal);

    if (branchAddDialog.exec() == QDialog::Accepted && m_model) {
        QModelIndex idx = m_model->addBranch(branchAddDialog.branchName(), branchAddDialog.track(), trackedIndex);
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
    QModelIndex idx = selectedIndex();

    const QString currentBranch = m_model->branchName(m_model->currentBranch());
    const QString nextBranch = m_model->branchName(idx);
    const QString popMessageStart = QCoreApplication::applicationName() +
            QLatin1String(" ") + nextBranch + QLatin1String("-AutoStash ");

    BranchCheckoutDialog branchCheckoutDialog(this, currentBranch, nextBranch);
    GitClient *gitClient = GitPlugin::instance()->gitClient();

    if (gitClient->gitStatus(m_repository, StatusMode(NoUntracked | NoSubmodules)) != GitClient::StatusChanged)
        branchCheckoutDialog.foundNoLocalChanges();

    QList<Stash> stashes;
    gitClient->synchronousStashList(m_repository, &stashes);
    foreach (const Stash &stash, stashes) {
        if (stash.message.startsWith(popMessageStart)) {
            branchCheckoutDialog.foundStashForNextBranch();
            break;
        }
    }

    if (!branchCheckoutDialog.hasLocalChanges() &&
        !branchCheckoutDialog.hasStashForNextBranch()) {
        // No local changes and no Auto Stash - no need to open dialog
        m_model->checkoutBranch(idx);
    } else if (branchCheckoutDialog.exec() == QDialog::Accepted && m_model) {

        if (branchCheckoutDialog.makeStashOfCurrentBranch()) {
            if (gitClient->synchronousStash(m_repository,
                           currentBranch + QLatin1String("-AutoStash")).isEmpty()) {
                return;
            }
        } else if (branchCheckoutDialog.moveLocalChangesToNextBranch()) {
            if (!gitClient->beginStashScope(m_repository, QLatin1String("Checkout"), NoPrompt))
                return;
        } else if (branchCheckoutDialog.discardLocalChanges()) {
            if (!gitClient->synchronousReset(m_repository))
                return;
        }

        m_model->checkoutBranch(idx);

        QString stashName;
        gitClient->synchronousStashList(m_repository, &stashes);
        foreach (const Stash &stash, stashes) {
            if (stash.message.startsWith(popMessageStart)) {
                stashName = stash.name;
                break;
            }
        }

        if (branchCheckoutDialog.moveLocalChangesToNextBranch())
            gitClient->endStashScope(m_repository);
        else if (branchCheckoutDialog.popStashOfNextBranch())
            gitClient->synchronousStashRestore(m_repository, stashName, true);
    }
    enableButtons();
}

/* Prompt to delete a local branch and do so. */
void BranchDialog::remove()
{
    QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch()); // otherwise the button would not be enabled!

    QString branchName = m_model->branchName(selected);
    if (branchName.isEmpty())
        return;

    QString message = tr("Would you like to delete the branch '%1'?").arg(branchName);
    bool wasMerged = m_model->branchIsMerged(selected);
    if (!wasMerged)
        message = tr("Would you like to delete the <b>unmerged</b> branch '%1'?").arg(branchName);

    if (QMessageBox::question(this, tr("Delete Branch"), message, QMessageBox::Yes|QMessageBox::No,
                              wasMerged ? QMessageBox::Yes : QMessageBox::No) == QMessageBox::Yes)
        m_model->removeBranch(selected);
}

void BranchDialog::rename()
{
    QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch()); // otherwise the button would not be enabled!
    QTC_CHECK(m_model->isLocal(selected));           // otherwise the button would not be enabled!

    QString oldBranchName = m_model->branchName(selected);
    QStringList localNames = m_model->localBranchNames();

    BranchAddDialog branchAddDialog(false, this);
    branchAddDialog.setBranchName(oldBranchName);
    branchAddDialog.setTrackedBranchName(QString(), false);

    branchAddDialog.exec();

    if (branchAddDialog.result() == QDialog::Accepted && m_model) {
        if (branchAddDialog.branchName() == oldBranchName)
            return;
        if (localNames.contains(branchAddDialog.branchName())) {
            QMessageBox::critical(this, tr("Branch Exists"),
                                  tr("Local branch \'%1\' already exists.")
                                  .arg(branchAddDialog.branchName()));
            return;
        }
        m_model->renameBranch(oldBranchName, branchAddDialog.branchName());
        refresh();
    }
    enableButtons();
}

void BranchDialog::diff()
{
    QString branchName = m_model->branchName(selectedIndex());
    if (branchName.isEmpty())
        return;
    // Do not pass working dir by reference since it might change
    GitPlugin::instance()->gitClient()->diffBranch(QString(m_repository), QStringList(), branchName);
}

void BranchDialog::log()
{
    QString branchName = m_model->branchName(selectedIndex());
    if (branchName.isEmpty())
        return;
    // Do not pass working dir by reference since it might change
    GitPlugin::instance()->gitClient()->log(QString(m_repository), QStringList(), false, QStringList(branchName));
}

void BranchDialog::merge()
{
    QModelIndex idx = selectedIndex();
    QTC_CHECK(idx != m_model->currentBranch()); // otherwise the button would not be enabled!

    const QString branch = m_model->branchName(idx);
    GitClient *client = GitPlugin::instance()->gitClient();
    if (client->beginStashScope(m_repository, QLatin1String("merge"), AllowUnstashed))
        client->synchronousMerge(m_repository, branch);
}

void BranchDialog::rebase()
{
    QModelIndex idx = selectedIndex();
    QTC_CHECK(idx != m_model->currentBranch()); // otherwise the button would not be enabled!

    const QString baseBranch = m_model->branchName(idx);
    GitClient *client = GitPlugin::instance()->gitClient();
    if (client->beginStashScope(m_repository, QLatin1String("rebase")))
        client->rebase(m_repository, baseBranch);
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
