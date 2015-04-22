/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PROJECTTREEWIDGET_H
#define PROJECTTREEWIDGET_H

#include "expanddata.h"

#include <coreplugin/inavigationwidgetfactory.h>

#include <utils/fileutils.h>

#include <QWidget>
#include <QModelIndex>
#include <QSet>

QT_FORWARD_DECLARE_CLASS(QTreeView)

namespace ProjectExplorer {

class Project;
class Node;
class FolderNode;
class FileNode;

namespace Internal {

class FlatModel;

class ProjectTreeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectTreeWidget(QWidget *parent = 0);
    ~ProjectTreeWidget();

    bool autoSynchronization() const;
    void setAutoSynchronization(bool sync);
    bool projectFilter();
    bool generatedFilesFilter();
    QToolButton *toggleSync();
    Node *currentNode();
    void sync(ProjectExplorer::Node *node);

    static Node *nodeForFile(const Utils::FileName &fileName);
    static Node *mostExpandedNode(const QList<Node*> &nodes);

public slots:
    void toggleAutoSynchronization();
    void editCurrentItem();
    void collapseAll();

private slots:
    void setProjectFilter(bool filter);
    void setGeneratedFilesFilter(bool filter);

    void handleCurrentItemChange(const QModelIndex &current);
    void showContextMenu(const QPoint &pos);
    void openItem(const QModelIndex &mainIndex);
    void handleProjectAdded(ProjectExplorer::Project *project);
    void startupProjectChanged(ProjectExplorer::Project *project);
    void initView();

    void loadExpandData();
    void saveExpandData();
    void disableAutoExpand();

private:
    void setCurrentItem(ProjectExplorer::Node *node);
    void recursiveLoadExpandData(const QModelIndex &index, QSet<ExpandData> &data);
    void recursiveSaveExpandData(const QModelIndex &index, QList<QVariant> *data);
    static int expandedCount(Node *node);
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void renamed(const Utils::FileName &oldPath, const Utils::FileName &newPath);

    QSet<ExpandData> m_toExpand;
    QTreeView *m_view;
    FlatModel *m_model;
    QAction *m_filterProjectsAction;
    QAction *m_filterGeneratedFilesAction;
    QToolButton *m_toggleSync;

    QModelIndex m_subIndex;
    QString m_modelId;
    bool m_autoSync;
    bool m_autoExpand;
    Utils::FileName m_delayedRename;

    static QList<ProjectTreeWidget *> m_projectTreeWidgets;
    friend class ProjectTreeWidgetFactory;
};

class ProjectTreeWidgetFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT
public:
    ProjectTreeWidgetFactory();

    Core::NavigationView createWidget();
    void restoreSettings(int position, QWidget *widget);
    void saveSettings(int position, QWidget *widget);
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTTREEWIDGET_H
