// Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/inavigationwidgetfactory.h>

#include <utils/filepath.h>

#include <QHash>
#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QModelIndex;
class QPoint;
class QToolButton;
class QTreeView;

namespace QtTaskTree { class Group; }
QT_END_NAMESPACE

namespace Utils {
class ElidingLabel;
class FancyLineEdit;
class NavigationTreeView;
} // Utils

namespace Git::Internal {

class BranchModel;
class BranchFilterModel;
class BranchViewFactory;

class BranchView : public QWidget
{
    Q_OBJECT
public:
    explicit BranchView(BranchViewFactory *factory = nullptr);

    QList<QToolButton *> createToolButtons();

protected:
    void showEvent(QShowEvent *) override;

private:
#ifdef WITH_TESTS
    friend class BranchViewTest;
#endif
    void refresh();
    void resizeColumns();
    void slotCustomContextMenu(const QPoint &point);
    void expandAndResize();
    void saveTreeState();
    void restoreTreeState();
    QString pathElementForIndex(const QModelIndex &index) const;
    QString pathForIndex(const QModelIndex &index) const;
    QModelIndex sourceIndexForPath(const QString &path) const;
    void setIncludeOldEntries(bool filter);
    void setIncludeTags(bool includeTags);
    QModelIndex selectedIndex();
    bool add();
    bool checkout();
    bool remove();
    bool rename();
    void reset(const QByteArray &resetType);
    QtTaskTree::Group fastForwardMergeRecipe(const std::function<void()> &callback);
    void merge(const QString &branch, bool allowFastForward);
    void rebase(const QString &branch);
    void cherryPick();
    void log(const QModelIndex &idx);
    void reflog(const QModelIndex &idx);
    void push();

    QAction *m_includeOldEntriesAction = nullptr;
    QAction *m_includeTagsAction = nullptr;
    QAction *m_addAction = nullptr;
    QAction *m_refreshAction = nullptr;

    Utils::ElidingLabel *m_repositoryLabel = nullptr;
    Utils::FancyLineEdit *m_filterEdit = nullptr;
    Utils::NavigationTreeView *m_branchView = nullptr;
    BranchModel *m_model = nullptr;
    BranchFilterModel *m_filterModel = nullptr;
    BranchViewFactory *m_factory = nullptr;
    QHash<QString, bool> m_expandedPaths;
    QString m_currentPath;
    Utils::FilePath m_treeStateRepository;
    bool m_hasSavedTreeState = false;
};

class BranchViewFactory : public Core::INavigationWidgetFactory
{
public:
    BranchViewFactory();
    ~BranchViewFactory() override;

    void refresh(const Utils::FilePath &repository);
    void refreshIfSame(const Utils::FilePath &repository);
    void refreshCurrentBranch();
    void viewShown(BranchView *view);

private:
    Core::NavigationView createWidget() override;
    bool hasVisibleView() const;

    QList<QPointer<BranchView>> m_views;
    Utils::FilePath m_repository;
    bool m_needsRefresh = true;
};

#ifdef WITH_TESTS
QObject *createBranchViewTest();
#endif

} // Git::Internal
