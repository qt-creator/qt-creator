// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/inavigationwidgetfactory.h>

#include <utils/filepath.h>

#include <QWidget>
#include <QModelIndex>

QT_FORWARD_DECLARE_CLASS(QTreeView)

namespace ProjectExplorer {

class Node;

namespace Internal {

class FlatModel;

class ProjectTreeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectTreeWidget(QWidget *parent = nullptr);
    ~ProjectTreeWidget() override;

    bool autoSynchronization() const;
    void setAutoSynchronization(bool sync);
    bool projectFilter();
    bool generatedFilesFilter();
    bool disabledFilesFilter();
    bool trimEmptyDirectoriesFilter();
    bool hideSourceGroups();
    Node *currentNode();
    void sync(ProjectExplorer::Node *node);
    void showMessage(ProjectExplorer::Node *node, const QString &message);

    static Node *nodeForFile(const Utils::FilePath &fileName);

    void toggleAutoSynchronization();
    void editCurrentItem();
    void expandCurrentNodeRecursively();
    void collapseAll();
    void expandAll();

    QList<QToolButton *> createToolButtons();

private:
    void setProjectFilter(bool filter);
    void setGeneratedFilesFilter(bool filter);
    void setDisabledFilesFilter(bool filter);
    void setTrimEmptyDirectories(bool filter);
    void setHideSourceGroups(bool filter);

    void handleCurrentItemChange(const QModelIndex &current);
    void showContextMenu(const QPoint &pos);
    void openItem(const QModelIndex &mainIndex);

    void setCurrentItem(ProjectExplorer::Node *node);
    static int expandedCount(Node *node);
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void renamed(const Utils::FilePath &oldPath, const Utils::FilePath &newPath);

    void syncFromDocumentManager();

    void expandNodeRecursively(const QModelIndex &index);

    QTreeView *m_view = nullptr;
    FlatModel *m_model = nullptr;
    QAction *m_filterProjectsAction = nullptr;
    QAction *m_filterGeneratedFilesAction = nullptr;
    QAction *m_filterDisabledFilesAction = nullptr;
    QAction *m_trimEmptyDirectoriesAction = nullptr;
    QAction *m_toggleSync = nullptr;
    QAction *m_hideSourceGroupsAction = nullptr;

    QString m_modelId;
    bool m_autoSync = true;
    QList<Utils::FilePath> m_delayedRename;

    static QList<ProjectTreeWidget *> m_projectTreeWidgets;
    friend class ProjectTreeWidgetFactory;
};

class ProjectTreeWidgetFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT
public:
    ProjectTreeWidgetFactory();

    Core::NavigationView createWidget() override;
    void restoreSettings(Utils::QtcSettings *settings, int position, QWidget *widget) override;
    void saveSettings(Utils::QtcSettings *settings, int position, QWidget *widget) override;
};

} // namespace Internal
} // namespace ProjectExplorer
