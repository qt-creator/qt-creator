// Copyright (C) 2026 The Qt Company Ltd.
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
QT_END_NAMESPACE

namespace Utils {
class ElidingLabel;
class NavigationTreeView;
} // Utils

namespace Git::Internal {

class GitGraphFilterModel;
class GitGraphModel;

class GitGraphView : public QWidget
{
public:
    GitGraphView();

    void refreshIfSame(const Utils::FilePath &repository);
    void refresh(const Utils::FilePath &repository, bool force);
    void refreshCurrentBranch();

    void setAllBranches(bool allBranches);
    bool allBranches() const;

    QList<QToolButton *> createToolButtons();

protected:
    void showEvent(QShowEvent *) override;

private:
    void refreshCurrentRepository();
    void loadMoreIfAtEnd();
    void showCommit(const QModelIndex &index);
    void showFileChanges(const QString &hash, const QString &file, const QString &oldFile);
    void slotCustomContextMenu(const QPoint &point);
    QString hashAt(const QModelIndex &filteredIndex) const;

    QAction *m_refreshAction = nullptr;
    QAction *m_allBranchesAction = nullptr;
    Utils::ElidingLabel *m_repositoryLabel = nullptr;
    Utils::NavigationTreeView *m_graphView = nullptr;
    GitGraphModel *m_model = nullptr;
    GitGraphFilterModel *m_filterModel = nullptr;
    Utils::FilePath m_repository;
    bool m_blockRefresh = false;
    bool m_postponedRefresh = false;
};

class GitGraphViewFactory : public Core::INavigationWidgetFactory
{
public:
    GitGraphViewFactory();

    GitGraphView *view() const;

private:
    Core::NavigationView createWidget() override;
    void saveSettings(Utils::QtcSettings *settings, int position, QWidget *widget) override;
    void restoreSettings(Utils::QtcSettings *settings, int position, QWidget *widget) override;

    QPointer<GitGraphView> m_view;
};

} // Git::Internal
