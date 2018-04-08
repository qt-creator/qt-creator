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
#include "branchutils.h"
#include "gitclient.h"
#include "gitplugin.h"
#include "gitutils.h"
#include "gitconstants.h"
#include "ui_branchdialog.h"

#include <vcsbase/vcsoutputwindow.h>
#include <coreplugin/documentmanager.h>

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
    BranchUtils(this),
    m_ui(new Ui::BranchDialog)
{
    setModal(false);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setAttribute(Qt::WA_DeleteOnClose, true); // Do not update unnecessarily

    m_ui->setupUi(this);
    setBranchView(m_ui->branchView);
    m_ui->includeOldCheckBox->setToolTip(
                tr("Include branches and tags that have not been active for %n days.", 0,
                   Constants::OBSOLETE_COMMIT_AGE_IN_DAYS));
    m_ui->includeTagsCheckBox->setChecked(GitPlugin::client()->settings().boolValue(
                                              GitSettings::showTagsKey));

    connect(m_ui->refreshButton, &QAbstractButton::clicked, this, &BranchDialog::refreshCurrentRepository);
    connect(m_ui->addButton, &QAbstractButton::clicked, this, [this] { add(); });
    connect(m_ui->checkoutButton, &QAbstractButton::clicked, this, [this] { checkout(); });
    connect(m_ui->removeButton, &QAbstractButton::clicked, this, [this] { remove(); });
    connect(m_ui->renameButton, &QAbstractButton::clicked, this, &BranchDialog::rename);
    connect(m_ui->diffButton, &QAbstractButton::clicked, this, &BranchDialog::diff);
    connect(m_ui->logButton, &QAbstractButton::clicked, this, &BranchDialog::log);
    connect(m_ui->resetButton, &QAbstractButton::clicked, this, [this] { reset(); });
    connect(m_ui->mergeButton, &QAbstractButton::clicked, this, &BranchDialog::merge);
    connect(m_ui->rebaseButton, &QAbstractButton::clicked, this, [this] { rebase(); });
    connect(m_ui->cherryPickButton, &QAbstractButton::clicked, this, [this] { cherryPick(); });
    connect(m_ui->trackButton, &QAbstractButton::clicked, this, &BranchDialog::setRemoteTracking);
    connect(m_ui->includeOldCheckBox, &QCheckBox::toggled, this, [this](bool value) {
        m_model->setOldBranchesIncluded(value);
        refreshCurrentRepository();
    });
    connect(m_ui->includeTagsCheckBox, &QCheckBox::toggled, this, [this](bool value) {
        GitPlugin::client()->settings().setValue(GitSettings::showTagsKey, value);
        refreshCurrentRepository();
    });

    m_ui->branchView->setModel(m_model);
    m_ui->branchView->setFocus();

    connect(m_ui->branchView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &BranchDialog::enableButtons);
    connect(m_model, &QAbstractItemModel::dataChanged, this, &BranchDialog::resizeColumns);
    connect(m_model, &QAbstractItemModel::rowsInserted, this, &BranchDialog::resizeColumns);
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, &BranchDialog::resizeColumns);
    connect(m_model, &QAbstractItemModel::modelReset, this, &BranchDialog::expandAndResize);

    m_ui->branchView->selectionModel()->clear();
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
    m_ui->repositoryLabel->setText(GitPlugin::msgRepositoryLabel(m_repository));
    QString errorMessage;
    if (!m_model->refresh(m_repository, &errorMessage))
        VcsOutputWindow::appendError(errorMessage);
}

void BranchDialog::expandAndResize()
{
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

void BranchDialog::rename()
{
    if (BranchUtils::rename())
        refreshCurrentRepository();
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
    GitPlugin::client()->log(m_repository, QString(), false, {branchName});
}

void BranchDialog::merge()
{
    bool allowFastForward = true;
    if (isFastForwardMerge()) {
        QMenu popup;
        QAction *fastForward = popup.addAction(tr("Fast-Forward"));
        popup.addAction(tr("No Fast-Forward"));
        QAction *chosen = Utils::execMenuAtWidget(&popup, m_ui->mergeButton);
        if (!chosen)
            return;
        allowFastForward = (chosen == fastForward);
    }

    BranchUtils::merge(allowFastForward);
}

void BranchDialog::setRemoteTracking()
{
    m_model->setRemoteTracking(selectedIndex());
}

} // namespace Internal
} // namespace Git
