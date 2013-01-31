/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "pluginfilefactory.h"
#include "projectexplorer.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "iprojectmanager.h"
#include "session.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/messagemanager.h>

#include <QDebug>
#include <QMessageBox>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

/*!
    \class ProjectExplorer::Internal::ProjectFileFactory

    \brief Factory for project files.
*/

ProjectFileFactory::ProjectFileFactory(IProjectManager *manager)
  : m_mimeTypes(manager->mimeType()),
    m_manager(manager)
{
}

QStringList ProjectFileFactory::mimeTypes() const
{
    return m_mimeTypes;
}

Core::Id ProjectFileFactory::id() const
{
    return Core::Id(Constants::FILE_FACTORY_ID);
}

QString ProjectFileFactory::displayName() const
{
    return tr("Project File Factory", "ProjectExplorer::ProjectFileFactory display name.");
}

Core::IDocument *ProjectFileFactory::open(const QString &fileName)
{
    ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();
    QString errorMessage;
    pe->openProject(fileName, &errorMessage);
    if (!errorMessage.isEmpty())
        QMessageBox::critical(Core::ICore::mainWindow(), tr("Failed to open project"), errorMessage);
    return 0;
}

QList<ProjectFileFactory *> ProjectFileFactory::createFactories(QString *filterString)
{
    // Register factories for all project managers
    QList<Internal::ProjectFileFactory*> rc;
    QList<IProjectManager*> projectManagers =
        ExtensionSystem::PluginManager::getObjects<IProjectManager>();

    QList<Core::MimeGlobPattern> allGlobPatterns;

    const QString filterSeparator = QLatin1String(";;");
    filterString->clear();
    foreach (IProjectManager *manager, projectManagers) {
        rc.push_back(new ProjectFileFactory(manager));
        if (!filterString->isEmpty())
            *filterString += filterSeparator;
        const QString mimeType = manager->mimeType();
        Core::MimeType mime = Core::ICore::mimeDatabase()->findByType(mimeType);
        const QString pFilterString = mime.filterString();
        allGlobPatterns.append(mime.globPatterns());
        *filterString += pFilterString;
    }
    QString allProjectFilter = Core::MimeType::formatFilterString(tr("All Projects"), allGlobPatterns);
    allProjectFilter.append(filterSeparator);
    filterString->prepend(allProjectFilter);
    return rc;
}
