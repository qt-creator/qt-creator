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

#include "projectwindow.h"

#include "doubletabwidget.h"
#include "panelswidget.h"
#include "kitmanager.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectpanelfactory.h"
#include "session.h"
#include "target.h"

#include <coreplugin/idocument.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QStackedWidget>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
///
// ProjectWindow
///

ProjectWindow::ProjectWindow(QWidget *parent)
    : QWidget(parent),
      m_ignoreChange(false),
      m_currentWidget(0)
{
    // Setup overall layout:
    QVBoxLayout *viewLayout = new QVBoxLayout(this);
    viewLayout->setMargin(0);
    viewLayout->setSpacing(0);

    m_tabWidget = new DoubleTabWidget(this);
    viewLayout->addWidget(m_tabWidget);

    // Setup our container for the contents:
    m_centralWidget = new QStackedWidget(this);
    viewLayout->addWidget(m_centralWidget);

    // Connections
    connect(m_tabWidget, &DoubleTabWidget::currentIndexChanged,
            this, &ProjectWindow::showProperties);

    SessionManager *sessionManager = SessionManager::instance();
    connect(sessionManager, &SessionManager::projectAdded,
            this, &ProjectWindow::registerProject);
    connect(sessionManager, &SessionManager::aboutToRemoveProject,
            this, &ProjectWindow::deregisterProject);

    connect(sessionManager, &SessionManager::startupProjectChanged,
            this, &ProjectWindow::startupProjectChanged);

    connect(sessionManager, &SessionManager::projectDisplayNameChanged,
            this, &ProjectWindow::projectDisplayNameChanged);

    // Update properties to empty project for now:
    showProperties(-1, -1);
}

void ProjectWindow::aboutToShutdown()
{
    showProperties(-1, -1);
}

void ProjectWindow::removedTarget(Target *)
{
    Project *p = qobject_cast<Project *>(sender());
    QTC_ASSERT(p, return);
    if (p->targets().isEmpty())
        projectUpdated(p);
}

void ProjectWindow::projectUpdated(Project *project)
{
    // Called after a project was configured
    int currentIndex = m_tabWidget->currentIndex();
    int oldSubIndex = m_tabWidget->currentSubIndex();

    removeCurrentWidget();

    int newSubIndex = m_cache.recheckFactories(project, oldSubIndex);
    if (newSubIndex == -1)
        newSubIndex = 0;
    m_tabWidget->setSubTabs(currentIndex, m_cache.tabNames(project));
    m_ignoreChange = true;
    m_tabWidget->setCurrentIndex(currentIndex, newSubIndex);
    m_ignoreChange = false;

    QWidget *widget = m_cache.widgetFor(project, newSubIndex);
    if (widget) {
        m_currentWidget = widget;
        m_centralWidget->addWidget(m_currentWidget);
        m_centralWidget->setCurrentWidget(m_currentWidget);
        m_currentWidget->show();
    }
}

void ProjectWindow::projectDisplayNameChanged(Project *project)
{
    int index = m_cache.indexForProject(project);
    if (index < 0)
        return;

    m_ignoreChange = true;
    bool isCurrentIndex = m_tabWidget->currentIndex() == index;
    int subIndex = m_tabWidget->currentSubIndex();
    QStringList subTabs = m_tabWidget->subTabs(index);
    m_tabWidget->removeTab(index);

    m_cache.sort();

    int newIndex = m_cache.indexForProject(project);
    m_tabWidget->insertTab(newIndex, project->displayName(), project->projectFilePath().toString(), subTabs);

    if (isCurrentIndex)
        m_tabWidget->setCurrentIndex(newIndex, subIndex);
    m_ignoreChange = false;
}

void ProjectWindow::registerProject(Project *project)
{
    if (m_cache.isRegistered(project))
        return;

    m_cache.registerProject(project);
    m_tabWidget->insertTab(m_cache.indexForProject(project),
                           project->displayName(),
                           project->projectFilePath().toString(),
                           m_cache.tabNames(project));

    connect(project, &Project::removedTarget, this, &ProjectWindow::removedTarget);
}

bool ProjectWindow::deregisterProject(Project *project)
{
    int index = m_cache.indexForProject(project);
    if (index == -1)
        return false;

    disconnect(project, &Project::removedTarget, this, &ProjectWindow::removedTarget);

    QVector<QWidget *> deletedWidgets = m_cache.deregisterProject(project);
    if (deletedWidgets.contains(m_currentWidget))
        m_currentWidget = 0;

    m_tabWidget->removeTab(index);
    return true;
}

void ProjectWindow::startupProjectChanged(Project *p)
{
    int index = m_cache.indexForProject(p);
    if (index != -1)
        m_tabWidget->setCurrentIndex(index);
}

void ProjectWindow::showProperties(int index, int subIndex)
{
    if (m_ignoreChange)
        return;

    removeCurrentWidget();
    Project *project = m_cache.projectFor(index);
    if (!project) {
        return;
    }

    QWidget *widget = m_cache.widgetFor(project, subIndex);
    if (widget) {
        m_currentWidget = widget;
        m_centralWidget->addWidget(m_currentWidget);
        m_centralWidget->setCurrentWidget(m_currentWidget);
        m_currentWidget->show();
        if (hasFocus()) // we get assigned focus from setFocusToCurrentMode, pass that on
            m_currentWidget->setFocus();
    }

    SessionManager::setStartupProject(project);
}

void ProjectWindow::removeCurrentWidget()
{
    if (m_currentWidget) {
        m_centralWidget->removeWidget(m_currentWidget);
        m_currentWidget->hide();
        m_currentWidget = 0;
    }
}

// WidgetCache
void WidgetCache::registerProject(Project *project)
{
    QTC_ASSERT(!isRegistered(project), return);

    QList<ProjectPanelFactory *> fac = ProjectPanelFactory::factories();
    int factorySize = fac.size();

    ProjectInfo info;
    info.project = project;
    info.widgets.resize(factorySize);
    info.supports.resize(factorySize);

    for (int i = 0; i < factorySize; ++i)
        info.supports[i] = fac.at(i)->supports(project);

    m_projects.append(info);
    sort();
}

QVector<QWidget *> WidgetCache::deregisterProject(Project *project)
{
    QTC_ASSERT(isRegistered(project), return QVector<QWidget *>());

    int index = indexForProject(project);
    ProjectInfo info = m_projects.at(index);
    QVector<QWidget *> deletedWidgets = info.widgets;
    qDeleteAll(info.widgets);
    m_projects.removeAt(index);
    return deletedWidgets;
}

QStringList WidgetCache::tabNames(Project *project) const
{
    int index = indexForProject(project);
    if (index == -1)
        return QStringList();

    QList<ProjectPanelFactory *> fac = ProjectPanelFactory::factories();

    ProjectInfo info = m_projects.at(index);
    int end = info.supports.size();
    QStringList names;
    for (int i = 0; i < end; ++i)
        if (info.supports.at(i))
            names << fac.at(i)->displayName();
    return names;
}

int WidgetCache::factoryIndex(int projectIndex, int supportsIndex) const
{
    QList<ProjectPanelFactory *> fac = ProjectPanelFactory::factories();
    int end = fac.size();
    const ProjectInfo &info = m_projects.at(projectIndex);
    for (int i = 0; i < end; ++i) {
        if (info.supports.at(i)) {
            if (supportsIndex == 0)
                return i;
            else
                --supportsIndex;
        }
    }
    return -1;
}

QWidget *WidgetCache::widgetFor(Project *project, int supportsIndex)
{
    int projectIndex = indexForProject(project);
    if (projectIndex == -1)
        return 0;

    QList<ProjectPanelFactory *> fac = ProjectPanelFactory::factories();

    int factoryIdx = factoryIndex(projectIndex, supportsIndex);
    if (factoryIdx < 0 ||factoryIdx >= m_projects.at(projectIndex).widgets.size())
        return 0;
    if (!m_projects.at(projectIndex).widgets.at(factoryIdx))
        m_projects[projectIndex].widgets[factoryIdx] = fac.at(factoryIdx)->createWidget(project);
    return m_projects.at(projectIndex).widgets.at(factoryIdx);
}

bool WidgetCache::isRegistered(Project *project) const
{
    return Utils::anyOf(m_projects, [&project](ProjectInfo pinfo) {
        return pinfo.project == project;
    });
}

int WidgetCache::indexForProject(Project *project) const
{
    return Utils::indexOf(m_projects, [&project](ProjectInfo pinfo) {
        return pinfo.project == project;
    });
}

Project *WidgetCache::projectFor(int projectIndex) const
{
    if (projectIndex < 0)
        return 0;
    if (projectIndex >= m_projects.size())
        return 0;
    return m_projects.at(projectIndex).project;
}

void WidgetCache::sort()
{
    Utils::sort(m_projects, [](const ProjectInfo &a, const ProjectInfo &b) -> bool {
        QString aName = a.project->displayName();
        QString bName = b.project->displayName();
        if (aName == bName) {
            Utils::FileName aPath = a.project->projectFilePath();
            Utils::FileName bPath = b.project->projectFilePath();
            if (aPath == bPath)
                return a.project < b.project;
            else
                return aPath < bPath;
        } else {
            return aName < bName;
        }

    });
}

int WidgetCache::recheckFactories(Project *project, int oldSupportsIndex)
{
    int projectIndex = indexForProject(project);
    int factoryIdx = factoryIndex(projectIndex, oldSupportsIndex);

    ProjectInfo &info = m_projects[projectIndex];
    QList<ProjectPanelFactory *> fac = ProjectPanelFactory::factories();
    int end = fac.size();

    for (int i = 0; i < end; ++i) {
        info.supports[i] = fac.at(i)->supports(project);
        if (!info.supports.at(i)) {
            delete info.widgets.at(i);
            info.widgets[i] = 0;
        }
    }

    if (factoryIdx < 0)
        return -1;

    if (!info.supports.at(factoryIdx))
        return -1;

    int newIndex = 0;
    for (int i = 0; i < factoryIdx; ++i) {
        if (info.supports.at(i))
            ++newIndex;
    }
    return newIndex;
}

