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

#include "branchview.h"

#include "branchadddialog.h"
#include "branchcheckoutdialog.h"
#include "branchmodel.h"
#include "gitclient.h"
#include "gitconstants.h"
#include "gitplugin.h"
#include "gitutils.h"

#include <coreplugin/actionmanager/command.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/inavigationwidgetfactory.h>
#include <utils/elidinglabel.h>
#include <utils/navigationtreeview.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QDir>
#include <QLabel>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QPoint>
#include <QTreeView>
#include <QToolButton>
#include <QVBoxLayout>

using namespace Core;

namespace Git {
namespace Internal {

BranchView::BranchView() :
    BranchUtils(this),
    m_includeOldEntriesAction(new QAction(tr("Include Old Entries"), this)),
    m_includeTagsAction(new QAction(tr("Include Tags"), this)),
    m_addButton(new QToolButton(this)),
    m_refreshButton(new QToolButton(this)),
    m_repositoryLabel(new Utils::ElidingLabel(this)),
    m_branchView(new Utils::NavigationTreeView(this))
{
    setBranchView(m_branchView);
    m_addButton->setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    m_addButton->setToolTip(tr("Add Branch"));
    m_addButton->setProperty("noArrow", true);
    connect(m_addButton, &QToolButton::clicked, this, [this] { add(); });

    m_refreshButton->setIcon(Utils::Icons::RESET_TOOLBAR.icon());
    m_refreshButton->setToolTip(tr("Refresh"));
    m_refreshButton->setProperty("noArrow", true);
    connect(m_refreshButton, &QToolButton::clicked, this, &BranchView::refreshCurrentRepository);

    m_branchView->setModel(m_model);
    m_branchView->setHeaderHidden(true);
    setFocus();

    m_repositoryLabel->setElideMode(Qt::ElideLeft);
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_repositoryLabel);
    layout->addWidget(m_branchView);
    layout->setContentsMargins(0, 2, 0, 0);
    setLayout(layout);

    m_includeOldEntriesAction->setCheckable(true);
    m_includeOldEntriesAction->setToolTip(
                tr("Include branches and tags that have not been active for %n days.", nullptr,
                   Constants::OBSOLETE_COMMIT_AGE_IN_DAYS));
    connect(m_includeOldEntriesAction, &QAction::toggled,
            this, &BranchView::BranchView::setIncludeOldEntries);
    m_includeTagsAction->setCheckable(true);
    m_includeTagsAction->setChecked(
                GitPlugin::client()->settings().boolValue(GitSettings::showTagsKey));
    connect(m_includeTagsAction, &QAction::toggled,
            this, &BranchView::BranchView::setIncludeTags);

    m_branchView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_branchView, &QWidget::customContextMenuRequested,
            this, &BranchView::slotCustomContextMenu);
    connect(m_model, &QAbstractItemModel::modelReset,
            this, &BranchView::expandAndResize);

    m_branchView->selectionModel()->clear();
    m_repository = GitPlugin::instance()->currentState().topLevel();
    refreshCurrentRepository();
}

void BranchView::refreshIfSame(const QString &repository)
{
    if (m_repository == repository)
        refreshCurrentRepository();
}

void BranchView::refresh(const QString &repository, bool force)
{
    if (m_repository == repository && !force)
        return;

    m_repository = repository;
    m_repositoryLabel->setText(QDir::toNativeSeparators(m_repository));
    m_repositoryLabel->setToolTip(GitPlugin::msgRepositoryLabel(m_repository));
    QString errorMessage;
    if (!m_model->refresh(m_repository, &errorMessage))
        VcsBase::VcsOutputWindow::appendError(errorMessage);
}

QToolButton *BranchView::addButton() const
{
    return m_addButton;
}

QToolButton *BranchView::refreshButton() const
{
    return m_refreshButton;
}

void BranchView::refreshCurrentRepository()
{
    refresh(m_repository, true);
}

void BranchView::resizeColumns()
{
    for (int column = 0, total = m_model->columnCount(); column < total; ++column)
        m_branchView->resizeColumnToContents(column);
}

void BranchView::slotCustomContextMenu(const QPoint &point)
{
    const QModelIndex index = m_branchView->indexAt(point);
    if (!index.isValid())
        return;

    const QModelIndex currentBranch = m_model->currentBranch();
    const bool hasSelection = index.isValid();
    const bool currentSelected = hasSelection && index == currentBranch;
    const bool isLocal = m_model->isLocal(index);
    const bool isLeaf = m_model->isLeaf(index);
    const bool isTag = m_model->isTag(index);
    const bool hasActions = hasSelection && isLeaf;
    const bool currentLocal = m_model->isLocal(currentBranch);

    QMenu contextMenu;
    contextMenu.addAction(tr("Add..."), this, [this] { add(); });
    if (hasActions) {
        if (!currentSelected && (isLocal || isTag))
            contextMenu.addAction(tr("Remove"), this, [this] { remove(); });
        if (isLocal || isTag)
            contextMenu.addAction(tr("Rename"), this, [this] { rename(); });
        if (!currentSelected) {
            contextMenu.addAction(tr("Checkout"), this, [this] { checkout(); });
        }
        contextMenu.addSeparator();
        contextMenu.addAction(tr("Diff"), this, [this] {
            const QString fullName = m_model->fullName(selectedIndex(), true);
            if (!fullName.isEmpty())
                GitPlugin::client()->diffBranch(m_repository, fullName);
        });
        contextMenu.addAction(tr("Log"), this, [this] {
            const QString branchName = m_model->fullName(selectedIndex(), true);
            if (!branchName.isEmpty())
                GitPlugin::client()->log(m_repository, QString(), false, {branchName});
        });
        contextMenu.addSeparator();
        if (!currentSelected) {
            if (currentLocal)
                contextMenu.addAction(tr("Reset"), this, [this] { reset(); });
            QString mergeTitle;
            if (BranchUtils::isFastForwardMerge()) {
                contextMenu.addAction(tr("Merge (Fast-Forward)"), this, [this] { merge(true); });
                mergeTitle = tr("Merge (No Fast-Forward)");
            } else {
                mergeTitle = tr("Merge");
            }

            contextMenu.addAction(mergeTitle, this, [this] { merge(); });
            contextMenu.addAction(tr("Rebase"), this, [this] { rebase(); });
            contextMenu.addSeparator();
            contextMenu.addAction(tr("Cherry Pick"), this, [this] { cherryPick(); });
        }
        if (currentLocal && !currentSelected && !isTag) {
            contextMenu.addAction(tr("Track"), this, [this] {
                m_model->setRemoteTracking(selectedIndex());
            });
        }
    }
    contextMenu.exec(m_branchView->viewport()->mapToGlobal(point));
}

void BranchView::expandAndResize()
{
    m_branchView->expandAll();
    resizeColumns();
}

void BranchView::setIncludeOldEntries(bool filter)
{
    m_model->setOldBranchesIncluded(filter);
    refreshCurrentRepository();
}

void BranchView::setIncludeTags(bool includeTags)
{
    GitPlugin::client()->settings().setValue(GitSettings::showTagsKey, includeTags);
    refreshCurrentRepository();
}

BranchViewFactory::BranchViewFactory()
{
    setDisplayName(tr("Git Branches"));
    setPriority(500);
    setId(Constants::GIT_BRANCH_VIEW_ID);
}

NavigationView BranchViewFactory::createWidget()
{
    m_view = new BranchView;
    Core::NavigationView navigationView(m_view);

    auto filter = new QToolButton;
    filter->setIcon(Utils::Icons::FILTER.icon());
    filter->setToolTip(tr("Filter"));
    filter->setPopupMode(QToolButton::InstantPopup);
    filter->setProperty("noArrow", true);
    auto filterMenu = new QMenu(filter);
    filterMenu->addAction(m_view->m_includeOldEntriesAction);
    filterMenu->addAction(m_view->m_includeTagsAction);
    filter->setMenu(filterMenu);

    navigationView.dockToolBarWidgets << filter << m_view->addButton() << m_view->refreshButton();
    return navigationView;
}

BranchView *BranchViewFactory::view() const
{
    return m_view;
}

} // namespace Internal
} // namespace Git
