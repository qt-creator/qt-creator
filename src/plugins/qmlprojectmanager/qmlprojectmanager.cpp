/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmlprojectmanager.h"
#include "qmlprojectconstants.h"
#include "qmlproject.h"

#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <QtDebug>

using namespace QmlProjectManager;
using namespace QmlProjectManager::Internal;

Manager::Manager()
{
    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_projectContext  = uidm->uniqueIdentifier(QmlProjectManager::Constants::PROJECTCONTEXT);
    m_projectLanguage = uidm->uniqueIdentifier(QmlProjectManager::Constants::LANG_QML);
}

Manager::~Manager()
{ }

int Manager::projectContext() const
{ return m_projectContext; }

int Manager::projectLanguage() const
{ return m_projectLanguage; }

QString Manager::mimeType() const
{ return QLatin1String(Constants::QMLMIMETYPE); }

ProjectExplorer::Project *Manager::openProject(const QString &fileName)
{
    Core::MessageManager *messageManager = Core::ICore::instance()->messageManager();
    messageManager->displayStatusBarMessage(tr("Loading project %1 ...").arg(fileName), 50000);

    QFileInfo fileInfo(fileName);
    ProjectExplorer::ProjectExplorerPlugin *projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();

    foreach (ProjectExplorer::Project *pi, projectExplorer->session()->projects()) {
        if (fileName == pi->file()->fileName()) {
            messageManager->printToOutputPane(tr("Failed opening project '%1': Project already open").arg(QDir::toNativeSeparators(fileName)));
            messageManager->displayStatusBarMessage(tr("Failed opening project"), 5000);
            return 0;
        }
    }

    if (fileInfo.isFile()) {
        QmlProject *project = new QmlProject(this, fileName);
        messageManager->displayStatusBarMessage(tr("Done opening project"), 5000);
        return project;
    }

    return 0;
}

void Manager::registerProject(QmlProject *project)
{ m_projects.append(project); }

void Manager::unregisterProject(QmlProject *project)
{ m_projects.removeAll(project); }

void Manager::notifyChanged(const QString &fileName)
{
    foreach (QmlProject *project, m_projects) {
        if (fileName == project->filesFileName()) {
            project->refresh(QmlProject::Files);
        }
    }
}
