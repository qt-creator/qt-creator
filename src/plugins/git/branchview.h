// Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/inavigationwidgetfactory.h>

#include <utils/filepath.h>

#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QModelIndex;
class QPoint;
class QToolButton;
class QTreeView;
QT_END_NAMESPACE;

namespace Tasking { class TaskTree; }

namespace Utils {
class ElidingLabel;
class NavigationTreeView;
} // Utils

namespace Git::Internal {

class BranchModel;
class BranchFilterModel;

class BranchView : public QWidget
{
public:
    explicit BranchView();

    void refreshIfSame(const Utils::FilePath &repository);
    void refresh(const Utils::FilePath &repository, bool force);
    void refreshCurrentBranch();

    QList<QToolButton *> createToolButtons();

protected:
    void showEvent(QShowEvent *) override;

private:
    void refreshCurrentRepository();
    void resizeColumns();
    void slotCustomContextMenu(const QPoint &point);
    void expandAndResize();
    void setIncludeOldEntries(bool filter);
    void setIncludeTags(bool includeTags);
    QModelIndex selectedIndex();
    bool add();
    bool checkout();
    bool remove();
    bool rename();
    bool reset(const QByteArray &resetType);
    Tasking::TaskTree *onFastForwardMerge(const std::function<void()> &callback);
    bool merge(bool allowFastForward);
    void rebase();
    bool cherryPick();
    void log(const QModelIndex &idx);
    void reflog(const QModelIndex &idx);
    void push();

    QAction *m_includeOldEntriesAction = nullptr;
    QAction *m_includeTagsAction = nullptr;
    QAction *m_addAction = nullptr;
    QAction *m_refreshAction = nullptr;

    Utils::ElidingLabel *m_repositoryLabel = nullptr;
    Utils::NavigationTreeView *m_branchView = nullptr;
    BranchModel *m_model = nullptr;
    BranchFilterModel *m_filterModel = nullptr;
    Utils::FilePath m_repository;
    bool m_blockRefresh = false;
    bool m_postponedRefresh = false;
};

class BranchViewFactory : public Core::INavigationWidgetFactory
{
public:
    BranchViewFactory();

    BranchView *view() const;

private:
    Core::NavigationView createWidget() override;

    QPointer<BranchView> m_view;
};

} // Git::Internal
