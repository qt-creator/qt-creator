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
#include <utils/fancylineedit.h>
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
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QToolButton>
#include <QVBoxLayout>

using namespace Core;

namespace Git {
namespace Internal {

class BranchFilterModel : public QSortFilterProxyModel
{
public:
    BranchFilterModel(QObject *parent) : QSortFilterProxyModel(parent) {}
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        QAbstractItemModel *m = sourceModel();
        // Filter leaves only. The root node and all intermediate nodes should always be visible
        if (!sourceParent.isValid() || m->rowCount(m->index(sourceRow, 0, sourceParent)) > 0)
            return true;
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
};

BranchView::BranchView() :
    m_includeOldEntriesAction(new QAction(tr("Include Old Entries"), this)),
    m_includeTagsAction(new QAction(tr("Include Tags"), this)),
    m_addButton(new QToolButton(this)),
    m_refreshButton(new QToolButton(this)),
    m_repositoryLabel(new Utils::ElidingLabel(this)),
    m_branchView(new Utils::NavigationTreeView(this)),
    m_model(new BranchModel(GitPlugin::client(), this)),
    m_filterModel(new BranchFilterModel(this))
{
    m_addButton->setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    m_addButton->setProperty("noArrow", true);
    connect(m_addButton, &QToolButton::clicked, this, &BranchView::add);

    m_refreshButton->setIcon(Utils::Icons::RELOAD.icon());
    m_refreshButton->setToolTip(tr("Refresh"));
    m_refreshButton->setProperty("noArrow", true);
    connect(m_refreshButton, &QToolButton::clicked, this, &BranchView::refreshCurrentRepository);

    m_branchView->setHeaderHidden(true);
    setFocus();

    m_repositoryLabel->setElideMode(Qt::ElideLeft);

    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterRole(Qt::EditRole);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_branchView->setModel(m_filterModel);
    auto filterEdit = new Utils::FancyLineEdit(this);
    filterEdit->setFiltering(true);
    connect(filterEdit, &Utils::FancyLineEdit::textChanged,
            m_filterModel, QOverload<const QString &>::of(&BranchFilterModel::setFilterRegExp));
    auto layout = new QVBoxLayout(this);
    layout->addWidget(filterEdit);
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
    m_branchView->setEditTriggers(QAbstractItemView::SelectedClicked
                                  | QAbstractItemView::EditKeyPressed);
    m_branchView->setItemDelegate(new BranchValidationDelegate(this, m_model));
    connect(m_branchView, &QAbstractItemView::doubleClicked,
            this, [this](const QModelIndex &idx) { log(m_filterModel->mapToSource(idx)); });
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
    if (m_repository.isEmpty()) {
        m_repositoryLabel->setText(tr("<No repository>"));
        m_addButton->setToolTip(tr("Create Git Repository..."));
        m_branchView->setEnabled(false);
    } else {
        m_repositoryLabel->setText(QDir::toNativeSeparators(m_repository));
        m_repositoryLabel->setToolTip(GitPlugin::msgRepositoryLabel(m_repository));
        m_addButton->setToolTip(tr("Add Branch..."));
        m_branchView->setEnabled(true);
    }
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
    const QModelIndex filteredIndex = m_branchView->indexAt(point);
    if (!filteredIndex.isValid())
        return;

    const QModelIndex index = m_filterModel->mapToSource(filteredIndex);
    const QModelIndex currentBranch = m_model->currentBranch();
    const bool currentSelected = index.sibling(index.row(), 0) == currentBranch;
    const bool isLocal = m_model->isLocal(index);
    const bool isTag = m_model->isTag(index);
    const bool hasActions = m_model->isLeaf(index);
    const bool currentLocal = m_model->isLocal(currentBranch);

    QMenu contextMenu;
    contextMenu.addAction(tr("Add..."), this, &BranchView::add);
    const Utils::optional<QString> remote = m_model->remoteName(index);
    if (remote.has_value()) {
        contextMenu.addAction(tr("Fetch"), this, [this, &remote]() {
            GitPlugin::client()->fetch(m_repository, *remote);
        });
        contextMenu.addSeparator();
        contextMenu.addAction(tr("Manage Remotes..."), GitPlugin::instance(),
                              &GitPlugin::manageRemotes);
    }
    if (hasActions) {
        if (!currentSelected && (isLocal || isTag))
            contextMenu.addAction(tr("Remove..."), this, &BranchView::remove);
        if (isLocal || isTag)
            contextMenu.addAction(tr("Rename..."), this, &BranchView::rename);
        if (!currentSelected)
            contextMenu.addAction(tr("Checkout"), this, &BranchView::checkout);
        contextMenu.addSeparator();
        contextMenu.addAction(tr("Diff"), this, [this] {
            const QString fullName = m_model->fullName(selectedIndex(), true);
            if (!fullName.isEmpty())
                GitPlugin::client()->diffBranch(m_repository, fullName);
        });
        contextMenu.addAction(tr("Log"), this, [this] { log(selectedIndex()); });
        contextMenu.addSeparator();
        if (!currentSelected) {
            if (currentLocal)
                contextMenu.addAction(tr("Reset"), this, &BranchView::reset);
            QString mergeTitle;
            if (isFastForwardMerge()) {
                contextMenu.addAction(tr("Merge (Fast-Forward)"), this, [this] { merge(true); });
                mergeTitle = tr("Merge (No Fast-Forward)");
            } else {
                mergeTitle = tr("Merge");
            }

            contextMenu.addAction(mergeTitle, this, [this] { merge(false); });
            contextMenu.addAction(tr("Rebase"), this, &BranchView::rebase);
            contextMenu.addSeparator();
            contextMenu.addAction(tr("Cherry Pick"), this, &BranchView::cherryPick);
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

QModelIndex BranchView::selectedIndex()
{
    QModelIndexList selected = m_branchView->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
        return QModelIndex();
    return m_filterModel->mapToSource(selected.at(0));
}

bool BranchView::add()
{
    if (m_repository.isEmpty()) {
        GitPlugin::instance()->initRepository();
        return true;
    }

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

    BranchAddDialog branchAddDialog(localNames, true, this);
    branchAddDialog.setBranchName(suggestedName);
    branchAddDialog.setTrackedBranchName(isTag ? QString() : trackedBranch, !isLocal);

    if (branchAddDialog.exec() == QDialog::Accepted) {
        QModelIndex idx = m_model->addBranch(branchAddDialog.branchName(), branchAddDialog.track(),
                                             trackedIndex);
        if (!idx.isValid())
            return false;
        QModelIndex mappedIdx = m_filterModel->mapFromSource(idx);
        QTC_ASSERT(m_branchView, return false);
        m_branchView->selectionModel()->select(mappedIdx, QItemSelectionModel::Clear
                                             | QItemSelectionModel::Select
                                             | QItemSelectionModel::Current);
        m_branchView->scrollTo(mappedIdx);
        if (QMessageBox::question(this, tr("Checkout"), tr("Checkout branch?"),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            return checkout();
        }
    }

    return false;
}

bool BranchView::checkout()
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return false;

    const QModelIndex selected = selectedIndex();
    const QString currentBranch = m_model->fullName(m_model->currentBranch());
    const QString nextBranch = m_model->fullName(selected);
    const QString popMessageStart = QCoreApplication::applicationName() +
            ' ' + nextBranch + "-AutoStash ";

    BranchCheckoutDialog branchCheckoutDialog(this, currentBranch, nextBranch);
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
bool BranchView::remove()
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

    if (QMessageBox::question(this, isTag ? tr("Delete Tag") : tr("Delete Branch"),
                              message, QMessageBox::Yes | QMessageBox::No,
                              wasMerged ? QMessageBox::Yes : QMessageBox::No) == QMessageBox::Yes) {
        if (isTag)
            m_model->removeTag(selected);
        else
            m_model->removeBranch(selected);
    }

    return true;
}

bool BranchView::rename()
{
    const QModelIndex selected = selectedIndex();
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

bool BranchView::reset()
{
    const QString currentName = m_model->fullName(m_model->currentBranch());
    const QString branchName = m_model->fullName(selectedIndex());
    if (currentName.isEmpty() || branchName.isEmpty())
        return false;

    if (QMessageBox::question(this, tr("Git Reset"), tr("Hard reset branch \"%1\" to \"%2\"?")
                              .arg(currentName).arg(branchName),
                              QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        GitPlugin::client()->reset(m_repository, "--hard", branchName);
        return true;
    }
    return false;
}

bool BranchView::isFastForwardMerge()
{
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    const QString branch = m_model->fullName(selected, true);
    return GitPlugin::client()->isFastForwardMerge(m_repository, branch);
}

bool BranchView::merge(bool allowFastForward)
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

void BranchView::rebase()
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

bool BranchView::cherryPick()
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return false;
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    const QString branch = m_model->fullName(selected, true);
    return GitPlugin::client()->synchronousCherryPick(m_repository, branch);
}

void BranchView::log(const QModelIndex &idx)
{
    const QString branchName = m_model->fullName(idx, true);
    if (!branchName.isEmpty())
        GitPlugin::client()->log(m_repository, QString(), false, {branchName});
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
