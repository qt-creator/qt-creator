/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef PROJECTTREEWIDGET_H
#define PROJECTTREEWIDGET_H

#include <coreplugin/inavigationwidgetfactory.h>

#include <QtGui/QWidget>
#include <QtGui/QTreeView>

namespace Core {
class ICore;
}

namespace ProjectExplorer {

class ProjectExplorerPlugin;
class Project;
class Node;

namespace Internal {

class FlatModel;

class ProjectTreeWidget : public QWidget {
    Q_OBJECT
public:
    ProjectTreeWidget(Core::ICore *core, QWidget *parent = 0);

    bool autoSynchronization() const;
    void setAutoSynchronization(bool sync, bool syncNow = true);
    bool projectFilter();
    bool generatedFilesFilter();
    QToolButton *toggleSync();

public slots:
    void toggleAutoSynchronization();
    void editCurrentItem();

private slots:
    void setCurrentItem(ProjectExplorer::Node *node, ProjectExplorer::Project *project);
    void setProjectFilter(bool filter);
    void setGeneratedFilesFilter(bool filter);

    void handleCurrentItemChange(const QModelIndex &current);
    void showContextMenu(const QPoint &pos);
    void openItem(const QModelIndex &mainIndex);
    void handleProjectAdded(ProjectExplorer::Project *project);
    void startupProjectChanged(ProjectExplorer::Project *project);
    void initView();

private:
    Core::ICore *m_core;
    ProjectExplorerPlugin *m_explorer;
    QTreeView *m_view;
    FlatModel *m_model;
    QAction *m_filterProjectsAction;
    QAction *m_filterGeneratedFilesAction;
    QToolButton *m_toggleSync;

    QModelIndex m_subIndex;
    QString m_modelId;
    bool m_autoSync;

    friend class ProjectTreeWidgetFactory;
};

class ProjectTreeWidgetFactory : public Core::INavigationWidgetFactory
{
public:
    ProjectTreeWidgetFactory(Core::ICore *core);
    virtual ~ProjectTreeWidgetFactory();
    virtual QString displayName();
    virtual QKeySequence activationSequence();
    virtual Core::NavigationView createWidget();
    void restoreSettings(int position, QWidget *widget);
    void saveSettings(int position, QWidget *widget);
private:
    Core::ICore *m_core;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTTREEWIDGET_H
