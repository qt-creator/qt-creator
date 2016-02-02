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

    void aboutToShutdown();

    void projectUpdated(Project *project);

private:
    void projectDisplayNameChanged(Project *p);
    void showProperties(int index, int subIndex);
    void registerProject(Project*);
    bool deregisterProject(Project*);
    void startupProjectChanged(Project *);
    void removedTarget(Target*);

    void removeCurrentWidget();

    bool m_ignoreChange;
    DoubleTabWidget *m_tabWidget;
    QStackedWidget *m_centralWidget;
    QWidget *m_currentWidget;
    WidgetCache m_cache;
};

} // namespace Internal
} // namespace ProjectExplorer
