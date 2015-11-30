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

#include "pluginfilefactory.h"
#include "projectexplorer.h"
#include "project.h"
#include "iprojectmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

#include <QDebug>
#include <QMessageBox>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

/*!
    \class ProjectExplorer::Internal::ProjectFileFactory

    \brief The ProjectFileFactory class provides a factory for project files.
*/

ProjectFileFactory::ProjectFileFactory(IProjectManager *manager)
    : m_manager(manager)
{
    setId(Constants::FILE_FACTORY_ID);
    setDisplayName(tr("Project File Factory", "ProjectExplorer::ProjectFileFactory display name."));
    addMimeType(manager->mimeType());
}

Core::IDocument *ProjectFileFactory::open(const QString &fileName)
{
    QString errorMessage;
    ProjectExplorerPlugin::instance()->openProject(fileName, &errorMessage);
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
        Core::MimeType mime = Core::MimeDatabase::findByType(mimeType);
        const QString pFilterString = mime.filterString();
        allGlobPatterns.append(mime.globPatterns());
        *filterString += pFilterString;
    }
    QString allProjectFilter = Core::MimeType::formatFilterString(tr("All Projects"), allGlobPatterns);
    allProjectFilter.append(filterSeparator);
    filterString->prepend(allProjectFilter);
    return rc;
}
