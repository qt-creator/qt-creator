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

#ifndef PROJECTWINDOW_H
#define PROJECTWINDOW_H

#include "projectexplorer_export.h"

#include <QMap>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QStackedWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Project;
class Target;

namespace Internal {

class DoubleTabWidget;

class WidgetCache
{
public:
    void registerProject(Project *project);
    QVector<QWidget *> deregisterProject(Project *project);

    bool isRegistered(Project *project) const;
    int indexForProject(Project *project) const;
    Project *projectFor(int projectIndex) const;
    QStringList tabNames(Project *project) const;

    QWidget *widgetFor(Project *project, int factoryIndex);

    void sort();
    int recheckFactories(Project *project, int oldSupportsIndex);

    void clear();

private:
    int factoryIndex(int projectIndex, int supportsIndex) const;

    class ProjectInfo
    {
    public:
        Project *project;
        QVector<bool> supports;
        QVector<QWidget *> widgets;
    };
    QList<ProjectInfo> m_projects; //ordered by displaynames of the projects
};

class ProjectWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectWindow(QWidget *parent = 0);
    ~ProjectWindow();

    void aboutToShutdown();

public slots:
    void projectUpdated(ProjectExplorer::Project *project);

private slots:
    void projectDisplayNameChanged(ProjectExplorer::Project *p);
    void showProperties(int index, int subIndex);
    void registerProject(ProjectExplorer::Project*);
    bool deregisterProject(ProjectExplorer::Project*);
    void startupProjectChanged(ProjectExplorer::Project *);
    void removedTarget(ProjectExplorer::Target*);

private:
    void removeCurrentWidget();

    bool m_ignoreChange;
    DoubleTabWidget *m_tabWidget;
    QStackedWidget *m_centralWidget;
    QWidget *m_currentWidget;
    WidgetCache m_cache;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTWINDOW_H
