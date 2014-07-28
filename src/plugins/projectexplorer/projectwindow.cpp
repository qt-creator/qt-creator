/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    connect(m_tabWidget, SIGNAL(currentIndexChanged(int,int)),
            this, SLOT(showProperties(int,int)));

    QObject *sessionManager = SessionManager::instance();
    connect(sessionManager, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(registerProject(ProjectExplorer::Project*)));
    connect(sessionManager, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)),
            this, SLOT(deregisterProject(ProjectExplorer::Project*)));

    connect(sessionManager, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(startupProjectChanged(ProjectExplorer::Project*)));

    connect(sessionManager, SIGNAL(projectDisplayNameChanged(ProjectExplorer::Project*)),
            this, SLOT(projectDisplayNameChanged(ProjectExplorer::Project*)));

    // Update properties to empty project for now:
    showProperties(-1, -1);
}

ProjectWindow::~ProjectWindow()
{
}

void ProjectWindow::aboutToShutdown()
{
    showProperties(-1, -1); // that's a bit stupid, but otherwise stuff is still
                            // connected to the session
    disconnect(KitManager::instance(), 0, this, 0);
    disconnect(SessionManager::instance(), 0, this, 0);
}

void ProjectWindow::removedTarget(Target *)
{
    Project *p = qobject_cast<Project *>(sender());
    QTC_ASSERT(p, return);
    if (p->targets().isEmpty())
        projectUpdated(p);
}

void ProjectWindow::projectUpdated(Project *p)
{
    // Called after a project was configured
    int index = m_tabWidget->currentIndex();
    if (deregisterProject(p)) // might return false if the project is unloading
        registerProject(p);
    m_tabWidget->setCurrentIndex(index);
}

QStringList ProjectWindow::tabDisplayNamesFor(Project *project)
{
    QStringList subTabs;
    foreach (ProjectPanelFactory *panelFactory, ProjectPanelFactory::factories()) {
        if (panelFactory->supports(project))
            subTabs << panelFactory->displayName();
    }
    return subTabs;
}

int ProjectWindow::insertPosFor(Project *project)
{
    int newIndex = -1;
    for (int i = 0; i <= m_tabIndexToProject.count(); ++i) {
        if (i == m_tabIndexToProject.count() ||
            m_tabIndexToProject.at(i)->displayName() > project->displayName()) {
            newIndex = i;
            break;
        }
    }
    return newIndex;
}

void ProjectWindow::projectDisplayNameChanged(Project *project)
{
    int index = m_tabIndexToProject.indexOf(project);
    if (index < 0)
        return;

    m_ignoreChange = true;
    bool isCurrentIndex = m_tabWidget->currentIndex() == index;
    int subIndex = m_tabWidget->currentSubIndex();
    QStringList subTabs = m_tabWidget->subTabs(index);
    m_tabIndexToProject.removeAt(index);
    m_tabWidget->removeTab(index);

    int newIndex = insertPosFor(project);
    m_tabIndexToProject.insert(newIndex, project);
    m_tabWidget->insertTab(newIndex, project->displayName(), project->projectFilePath().toString(), subTabs);

    if (isCurrentIndex)
        m_tabWidget->setCurrentIndex(newIndex, subIndex);
    m_ignoreChange = false;
}

void ProjectWindow::registerProject(ProjectExplorer::Project *project)
{
    if (!project || m_tabIndexToProject.contains(project))
        return;

    // find index to insert:
    int index = insertPosFor(project);

    // Add the project specific pages
    QStringList subtabs = tabDisplayNamesFor(project);
    m_tabIndexToProject.insert(index, project);
    m_tabWidget->insertTab(index, project->displayName(), project->projectFilePath().toString(), subtabs);

    connect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
            this, SLOT(removedTarget(ProjectExplorer::Target*)));
}

bool ProjectWindow::deregisterProject(ProjectExplorer::Project *project)
{
    int index = m_tabIndexToProject.indexOf(project);
    if (index < 0)
        return false;

    m_tabIndexToProject.removeAt(index);
    m_tabWidget->removeTab(index);
    disconnect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
            this, SLOT(removedTarget(ProjectExplorer::Target*)));
    return true;
}

void ProjectWindow::startupProjectChanged(ProjectExplorer::Project *p)
{
    int index = m_tabIndexToProject.indexOf(p);
    if (index != -1)
        m_tabWidget->setCurrentIndex(index);
}

void ProjectWindow::showProperties(int index, int subIndex)
{
    if (m_ignoreChange)
        return;

    if (index < 0 || index >= m_tabIndexToProject.count()) {
        removeCurrentWidget();
        return;
    }

    Project *project = m_tabIndexToProject.at(index);

    // Set up custom panels again:
    int pos = 0;
    ProjectPanelFactory *fac = 0;

    foreach (ProjectPanelFactory *panelFactory, ProjectPanelFactory::factories()) {
        if (panelFactory->supports(project)) {
            if (subIndex == pos) {
                fac = panelFactory;
                break;
            }
            ++pos;
        }
    }

    if (fac) {
        removeCurrentWidget();

        QWidget *widget = fac->createWidget(project);
        Q_ASSERT(widget);

        m_currentWidget = widget;
        m_centralWidget->addWidget(m_currentWidget);
        m_centralWidget->setCurrentWidget(m_currentWidget);

    }

    SessionManager::setStartupProject(project);
}

void ProjectWindow::removeCurrentWidget()
{
    if (m_currentWidget) {
        m_centralWidget->removeWidget(m_currentWidget);
        if (m_currentWidget) {
            delete m_currentWidget;
            m_currentWidget = 0;
        }
    }
}
