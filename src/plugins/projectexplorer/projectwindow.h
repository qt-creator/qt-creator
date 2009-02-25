/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef PROJECTWINDOW_H
#define PROJECTWINDOW_H

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QModelIndex;
class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Project;
class PropertiesPanel;
class ProjectExplorerPlugin;
class SessionManager;

namespace Internal {

class ProjectWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectWindow(QWidget *parent = 0);
    ~ProjectWindow();

private slots:
    void showProperties(ProjectExplorer::Project *project, const QModelIndex &subIndex);
    void restoreStatus();
    void saveStatus();

    void updateTreeWidget();
    void handleItem(QTreeWidgetItem *item, int column);
    void handleCurrentItemChanged(QTreeWidgetItem *);

private:
    SessionManager *m_session;
    ProjectExplorerPlugin *m_projectExplorer;

    QTreeWidget* m_treeWidget;
    QTabWidget *m_panelsTabWidget;

    QList<PropertiesPanel*> m_panels;

    Project *findProject(const QString &path) const;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTWINDOW_H
