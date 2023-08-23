// Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "branchview.h"

#include "branchadddialog.h"
#include "branchcheckoutdialog.h"
#include "branchmodel.h"
#include "gitclient.h"
#include "gitconstants.h"
#include "gitplugin.h"
#include "gittr.h"
#include "gitutils.h"

#include <coreplugin/actionmanager/command.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/inavigationwidgetfactory.h>

#include <utils/elidinglabel.h>
#include <utils/fancylineedit.h>
#include <utils/navigationtreeview.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

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
using namespace Tasking;
using namespace Utils;
using namespace VcsBase;

namespace Git::Internal {

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

struct SetInContext
{
    SetInContext(bool &block) : m_block(block)
    {
        m_origValue = m_block;
        m_block = true;
    }
    ~SetInContext() { m_block = m_origValue; }
    bool &m_block;
    bool m_origValue;
};

BranchView::BranchView()
    : m_includeOldEntriesAction(new QAction(Tr::tr("Include Old Entries"), this))
    , m_includeTagsAction(new QAction(Tr::tr("Include Tags"), this))
    , m_addAction(new QAction(this))
    , m_refreshAction(new QAction(this))
    , m_repositoryLabel(new ElidingLabel(this))
    , m_branchView(new NavigationTreeView(this))
    , m_model(new BranchModel(this))
    , m_filterModel(new BranchFilterModel(this))
{
    m_addAction->setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    connect(m_addAction, &QAction::triggered, this, &BranchView::add);

    m_refreshAction->setIcon(Utils::Icons::RELOAD_TOOLBAR.icon());
    m_refreshAction->setToolTip(Tr::tr("Refresh"));
    connect(m_refreshAction, &QAction::triggered, this, &BranchView::refreshCurrentRepository);

    m_branchView->setHeaderHidden(true);
    setFocus();

    m_repositoryLabel->setElideMode(Qt::ElideLeft);

    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterRole(Qt::EditRole);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_branchView->setModel(m_filterModel);
    auto filterEdit = new FancyLineEdit(this);
    filterEdit->setFiltering(true);
    connect(filterEdit, &FancyLineEdit::textChanged,
            m_filterModel, QOverload<const QString &>::of(&BranchFilterModel::setFilterRegularExpression));
    auto layout = new QVBoxLayout(this);
    layout->addWidget(filterEdit);
    layout->addWidget(m_repositoryLabel);
    layout->addWidget(m_branchView);
    layout->setContentsMargins(0, 2, 0, 0);
    setLayout(layout);

    m_includeOldEntriesAction->setCheckable(true);
    m_includeOldEntriesAction->setToolTip(
                Tr::tr("Include branches and tags that have not been active for %n days.", nullptr,
                   Constants::OBSOLETE_COMMIT_AGE_IN_DAYS));
    connect(m_includeOldEntriesAction, &QAction::toggled,
            this, &BranchView::setIncludeOldEntries);
    m_includeTagsAction->setCheckable(true);
    m_includeTagsAction->setChecked(settings().showTags());
    connect(m_includeTagsAction, &QAction::toggled,
            this, &BranchView::setIncludeTags);

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
    m_repository = GitPlugin::currentState().topLevel();
}

void BranchView::refreshIfSame(const FilePath &repository)
{
    if (m_repository != repository)
        return;
    if (m_blockRefresh)
        m_postponedRefresh = true;
    else
        refreshCurrentRepository();
}

void BranchView::refresh(const FilePath &repository, bool force)
{
    if (m_blockRefresh || (m_repository == repository && !force))
        return;

    m_repository = repository;
    if (m_repository.isEmpty()) {
        m_repositoryLabel->setText(Tr::tr("<No repository>"));
        m_addAction->setToolTip(Tr::tr("Create Git Repository..."));
        m_branchView->setEnabled(false);
    } else {
        m_repositoryLabel->setText(m_repository.toUserOutput());
        m_repositoryLabel->setToolTip(GitPlugin::msgRepositoryLabel(m_repository));
        m_addAction->setToolTip(Tr::tr("Add Branch..."));
        m_branchView->setEnabled(true);
    }

    // Do not refresh the model when the view is hidden
    if (!isVisible())
        return;

    m_model->refresh(m_repository, BranchModel::ShowError::Yes);
}

void BranchView::refreshCurrentBranch()
{
    m_model->refreshCurrentBranch();
}

void BranchView::showEvent(QShowEvent *)
{
    refreshCurrentRepository();
}

QList<QToolButton *> BranchView::createToolButtons()
{
    auto filter = new QToolButton;
    filter->setIcon(Utils::Icons::FILTER.icon());
    filter->setToolTip(Tr::tr("Filter"));
    filter->setPopupMode(QToolButton::InstantPopup);
    filter->setProperty(StyleHelper::C_NO_ARROW, true);

    auto filterMenu = new QMenu(filter);
    filterMenu->addAction(m_includeOldEntriesAction);
    filterMenu->addAction(m_includeTagsAction);
    filter->setMenu(filterMenu);

    auto addButton = new QToolButton;
    addButton->setDefaultAction(m_addAction);
    addButton->setProperty(StyleHelper::C_NO_ARROW, true);

    auto refreshButton = new QToolButton;
    refreshButton->setDefaultAction(m_refreshAction);
    refreshButton->setProperty(StyleHelper::C_NO_ARROW, true);

    return {filter, addButton, refreshButton};
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
    const QString indexName = m_model->fullName(index);
    const QModelIndex currentBranch = m_model->currentBranch();
    const QString currentName = m_model->fullName(currentBranch);
    const bool currentSelected = index.sibling(index.row(), 0) == currentBranch;
    const bool isLocal = m_model->isLocal(index);
    const bool isTag = m_model->isTag(index);
    const bool hasActions = m_model->isLeaf(index);
    const bool currentLocal = m_model->isLocal(currentBranch);
    std::unique_ptr<TaskTree> taskTree;
    QAction *mergeAction = nullptr;

    SetInContext block(m_blockRefresh);
    QMenu contextMenu;
    contextMenu.addAction(Tr::tr("&Add..."), this, &BranchView::add);
    const std::optional<QString> remote = m_model->remoteName(index);
    if (remote.has_value()) {
        contextMenu.addAction(Tr::tr("&Fetch"), this, [this, &remote] {
            gitClient().fetch(m_repository, *remote);
        });
        contextMenu.addSeparator();
        if (!remote->isEmpty()) {
            contextMenu.addAction(Tr::tr("Remove &Stale Branches"), this, [this, &remote] {
                gitClient().removeStaleRemoteBranches(m_repository, *remote);
            });
            contextMenu.addSeparator();
        }
        contextMenu.addAction(Tr::tr("Manage &Remotes..."), this, [] {
            GitPlugin::manageRemotes();
        });
    }
    if (hasActions) {
        if (!currentSelected && (isLocal || isTag))
            contextMenu.addAction(Tr::tr("Rem&ove..."), this, &BranchView::remove);
        if (isLocal || isTag)
            contextMenu.addAction(Tr::tr("Re&name..."), this, &BranchView::rename);
        if (!currentSelected)
            contextMenu.addAction(Tr::tr("&Checkout"), this, &BranchView::checkout);
        contextMenu.addSeparator();
        contextMenu.addAction(Tr::tr("&Diff"), this, [this] {
            const QString fullName = m_model->fullName(selectedIndex(), true);
            if (!fullName.isEmpty())
                gitClient().diffBranch(m_repository, fullName);
        });
        contextMenu.addAction(Tr::tr("&Log"), this, [this] { log(selectedIndex()); });
        contextMenu.addAction(Tr::tr("Reflo&g"), this, [this] { reflog(selectedIndex()); });
        contextMenu.addSeparator();
        if (!currentSelected) {
            auto resetMenu = new QMenu(Tr::tr("Re&set"), &contextMenu);
            resetMenu->addAction(Tr::tr("&Hard"), this, [this] { reset("hard"); });
            resetMenu->addAction(Tr::tr("&Mixed"), this, [this] { reset("mixed"); });
            resetMenu->addAction(Tr::tr("&Soft"), this, [this] { reset("soft"); });
            contextMenu.addMenu(resetMenu);
            mergeAction = contextMenu.addAction(Tr::tr("&Merge \"%1\" into \"%2\"")
                                                    .arg(indexName, currentName),
                                                this,
                                                [this] { merge(false); });
            taskTree.reset(onFastForwardMerge([&] {
                auto ffMerge = new QAction(
                    Tr::tr("&Merge \"%1\" into \"%2\" (Fast-Forward)").arg(indexName, currentName));
                connect(ffMerge, &QAction::triggered, this, [this] { merge(true); });
                contextMenu.insertAction(mergeAction, ffMerge);
                mergeAction->setText(Tr::tr("Merge \"%1\" into \"%2\" (No &Fast-Forward)")
                                         .arg(indexName, currentName));
            }));
            connect(mergeAction, &QObject::destroyed, taskTree.get(), &TaskTree::stop);

            contextMenu.addAction(Tr::tr("&Rebase \"%1\" on \"%2\"")
                                  .arg(currentName, indexName),
                                  this, &BranchView::rebase);
            contextMenu.addSeparator();
            contextMenu.addAction(Tr::tr("Cherry &Pick"), this, &BranchView::cherryPick);
        }
        if (!currentSelected && !isTag) {
            if (currentLocal) {
                contextMenu.addAction(Tr::tr("&Track"), this, [this] {
                    m_model->setRemoteTracking(selectedIndex());
                });
            }
            if (!isLocal) {
                contextMenu.addSeparator();
                contextMenu.addAction(Tr::tr("&Push"), this, &BranchView::push);
            }
        }
    }
    contextMenu.exec(m_branchView->viewport()->mapToGlobal(point));
    if (m_postponedRefresh) {
        refreshCurrentRepository();
        m_postponedRefresh = false;
    }
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
    settings().showTags.setValue(includeTags);
    refreshCurrentRepository();
}

QModelIndex BranchView::selectedIndex()
{
    QModelIndexList selected = m_branchView->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
        return {};
    return m_filterModel->mapToSource(selected.at(0));
}

bool BranchView::add()
{
    if (m_repository.isEmpty()) {
        GitPlugin::initRepository();
        return true;
    }

    QModelIndex trackedIndex = selectedIndex();
    QString trackedBranch = m_model->fullName(trackedIndex);
    if (trackedBranch.isEmpty()) {
        trackedIndex = m_model->currentBranch();
        trackedBranch = m_model->fullName(trackedIndex);
    }
    const bool isLocal = m_model->isLocal(trackedIndex);
    const bool isTracked = !m_model->isHead(trackedIndex) && !m_model->isTag(trackedIndex);

    const QStringList localNames = m_model->localBranchNames();

    BranchAddDialog branchAddDialog(localNames, BranchAddDialog::Type::AddBranch, this);

    const QString suggestedName = GitClient::suggestedLocalBranchName(
                m_repository, localNames, trackedBranch,
                isTracked ? GitClient::BranchTargetType::Remote : GitClient::BranchTargetType::Commit);
    branchAddDialog.setBranchName(suggestedName);
    branchAddDialog.setTrackedBranchName(isTracked ? trackedBranch : QString(), !isLocal);
    branchAddDialog.setCheckoutVisible(true);

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
        if (branchAddDialog.checkout())
            return checkout();
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

    if (gitClient().gitStatus(m_repository, StatusMode(NoUntracked | NoSubmodules)) != GitClient::StatusChanged)
        branchCheckoutDialog.foundNoLocalChanges();

    QList<Stash> stashes;
    gitClient().synchronousStashList(m_repository, &stashes);
    for (const Stash &stash : std::as_const(stashes)) {
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
            if (gitClient().synchronousStash(m_repository, currentBranch + "-AutoStash").isEmpty())
                return false;
        } else if (branchCheckoutDialog.moveLocalChangesToNextBranch()) {
            if (!gitClient().beginStashScope(m_repository, "Checkout", NoPrompt))
                return false;
        } else if (branchCheckoutDialog.discardLocalChanges()) {
            if (!gitClient().synchronousReset(m_repository))
                return false;
        }

        const bool moveChanges = branchCheckoutDialog.moveLocalChangesToNextBranch();
        const bool popStash = branchCheckoutDialog.popStashOfNextBranch();
        const auto commandHandler = [=](const CommandResult &) {
            if (moveChanges) {
                gitClient().endStashScope(m_repository);
            } else if (popStash) {
                QList<Stash> stashes;
                QString stashName;
                gitClient().synchronousStashList(m_repository, &stashes);
                for (const Stash &stash : std::as_const(stashes)) {
                    if (stash.message.startsWith(popMessageStart)) {
                        stashName = stash.name;
                        break;
                    }
                }
                gitClient().synchronousStashRestore(m_repository, stashName, true);
            }
        };
        m_model->checkoutBranch(selected, this, commandHandler);
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

    const QString branchName = m_model->fullName(selected);
    if (branchName.isEmpty())
        return false;

    const bool isTag = m_model->isTag(selected);
    const bool wasMerged = isTag ? true : m_model->branchIsMerged(selected);
    QString message;
    if (isTag)
        message = Tr::tr("Would you like to delete the tag \"%1\"?").arg(branchName);
    else if (wasMerged)
        message = Tr::tr("Would you like to delete the branch \"%1\"?").arg(branchName);
    else
        message = Tr::tr("Would you like to delete the <b>unmerged</b> branch \"%1\"?").arg(branchName);

    if (QMessageBox::question(this, isTag ? Tr::tr("Delete Tag") : Tr::tr("Delete Branch"),
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

    const QString oldName = m_model->fullName(selected);
    QStringList localNames;
    if (!isTag)
        localNames = m_model->localBranchNames();

    const BranchAddDialog::Type type = isTag ? BranchAddDialog::Type::RenameTag
                                             : BranchAddDialog::Type::RenameBranch;
    BranchAddDialog branchAddDialog(localNames, type, this);
    branchAddDialog.setBranchName(oldName);

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

bool BranchView::reset(const QByteArray &resetType)
{
    const QString currentName = m_model->fullName(m_model->currentBranch());
    const QString branchName = m_model->fullName(selectedIndex());
    if (currentName.isEmpty() || branchName.isEmpty())
        return false;

    if (QMessageBox::question(this, Tr::tr("Git Reset"), Tr::tr("Reset branch \"%1\" to \"%2\"?")
                              .arg(currentName, branchName),
                              QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        gitClient().reset(m_repository, QLatin1String("--" + resetType), branchName);
        return true;
    }
    return false;
}

TaskTree *BranchView::onFastForwardMerge(const std::function<void()> &callback)
{
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    const QString branch = m_model->fullName(selected, true);

    struct FastForwardStorage
    {
        QString mergeBase;
        QString topRevision;
    };

    const TreeStorage<FastForwardStorage> storage;

    const auto setupMergeBase = [=](Process &process) {
        gitClient().setupCommand(process, m_repository, {"merge-base", "HEAD", branch});
    };
    const auto onMergeBaseDone = [storage](const Process &process) {
        storage->mergeBase = process.cleanedStdOut().trimmed();
    };

    const ProcessTask topRevisionProc = gitClient().topRevision(
        m_repository,
        [storage](const QString &revision, const QDateTime &) {
            storage->topRevision = revision;
        });

    const Group root {
        Tasking::Storage(storage),
        parallel,
        ProcessTask(setupMergeBase, onMergeBaseDone),
        topRevisionProc,
        onGroupDone([storage, callback] {
            if (storage->mergeBase == storage->topRevision)
                callback();
        })
    };
    auto taskTree = new TaskTree(root);
    taskTree->start();
    return taskTree;
}

bool BranchView::merge(bool allowFastForward)
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return false;
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    const QString branch = m_model->fullName(selected, true);
    if (gitClient().beginStashScope(m_repository, "merge", AllowUnstashed))
        return gitClient().synchronousMerge(m_repository, branch, allowFastForward);

    return false;
}

void BranchView::rebase()
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return;
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    const QString baseBranch = m_model->fullName(selected, true);
    if (gitClient().beginStashScope(m_repository, "rebase"))
        gitClient().rebase(m_repository, baseBranch);
}

bool BranchView::cherryPick()
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return false;
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    const QString branch = m_model->fullName(selected, true);
    return gitClient().synchronousCherryPick(m_repository, branch);
}

void BranchView::log(const QModelIndex &idx)
{
    const QString branchName = m_model->fullName(idx, true);
    if (branchName.isEmpty())
        return;
    SetInContext block(m_blockRefresh);
    gitClient().log(m_repository, QString(), false, {branchName});
}

void BranchView::reflog(const QModelIndex &idx)
{
    const QString branchName = m_model->fullName(idx, true);
    if (branchName.isEmpty())
        return;
    SetInContext block(m_blockRefresh);
    gitClient().reflog(m_repository, branchName);
}

void BranchView::push()
{
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    const QString fullTargetName = m_model->fullName(selected);
    const int pos = fullTargetName.indexOf('/');

    const QString localBranch = m_model->fullName(m_model->currentBranch());
    const QString remoteName = fullTargetName.left(pos);
    const QString remoteBranch = fullTargetName.mid(pos + 1);
    const QStringList pushArgs = {remoteName, localBranch + ':' + remoteBranch};

    gitClient().push(m_repository, pushArgs);
}

BranchViewFactory::BranchViewFactory()
{
    setDisplayName(Tr::tr("Git Branches"));
    setPriority(500);
    setId(Constants::GIT_BRANCH_VIEW_ID);
}

NavigationView BranchViewFactory::createWidget()
{
    m_view = new BranchView;
    return {m_view, m_view->createToolButtons()};
}

BranchView *BranchViewFactory::view() const
{
    return m_view;
}

} // Git::Internal
