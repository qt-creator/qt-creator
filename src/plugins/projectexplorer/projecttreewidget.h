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

#pragma once

#include "expanddata.h"

#include <coreplugin/inavigationwidgetfactory.h>

#include <utils/fileutils.h>

#include <QWidget>
#include <QModelIndex>
#include <QSet>

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
    QToolButton *toggleSync();
    Node *currentNode();
    void sync(ProjectExplorer::Node *node);
    void showMessage(ProjectExplorer::Node *node, const QString &message);

    static Node *nodeForFile(const Utils::FileName &fileName);

    void toggleAutoSynchronization();
    void editCurrentItem();
    void collapseAll();

private:
    void setProjectFilter(bool filter);
    void setGeneratedFilesFilter(bool filter);

    void handleCurrentItemChange(const QModelIndex &current);
    void showContextMenu(const QPoint &pos);
    void openItem(const QModelIndex &mainIndex);

    void setCurrentItem(ProjectExplorer::Node *node);
    static int expandedCount(Node *node);
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void renamed(const Utils::FileName &oldPath, const Utils::FileName &newPath);

    QTreeView *m_view = nullptr;
    FlatModel *m_model = nullptr;
    QAction *m_filterProjectsAction = nullptr;
    QAction *m_filterGeneratedFilesAction;
    QToolButton *m_toggleSync;

    QString m_modelId;
    bool m_autoSync = false;
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
    void restoreSettings(QSettings *settings, int position, QWidget *widget);
    void saveSettings(QSettings *settings, int position, QWidget *widget);
};

} // namespace Internal
} // namespace ProjectExplorer
