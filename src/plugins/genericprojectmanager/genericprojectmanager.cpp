/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "genericprojectmanager.h"
#include "genericprojectconstants.h"
#include "genericproject.h"

#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtDebug>

using namespace GenericProjectManager::Internal;

Manager::Manager()
{
    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_projectContext  = uidm->uniqueIdentifier(GenericProjectManager::Constants::PROJECTCONTEXT);
    m_projectLanguage = uidm->uniqueIdentifier(ProjectExplorer::Constants::LANG_CXX);
}

Manager::~Manager()
{ }

int Manager::projectContext() const
{ return m_projectContext; }

int Manager::projectLanguage() const
{ return m_projectLanguage; }

QString Manager::mimeType() const
{ return QLatin1String(Constants::GENERICMIMETYPE); }

ProjectExplorer::Project *Manager::openProject(const QString &fileName)
{
    QFileInfo fileInfo(fileName);

    if (fileInfo.isFile()) {
        GenericProject *project = new GenericProject(this, fileName);
        return project;
    }

    return 0;
}

void Manager::registerProject(GenericProject *project)
{ m_projects.append(project); }

void Manager::unregisterProject(GenericProject *project)
{ m_projects.removeAll(project); }

void Manager::notifyChanged(const QString &fileName)
{
    foreach (GenericProject *project, m_projects) {
        if (fileName == project->filesFileName()) {
            project->refresh(GenericProject::Files);
        }
        else if (fileName == project->includesFileName() ||
                 fileName == project->configFileName()) {
            project->refresh(GenericProject::Configuration);
        }
    }
}
