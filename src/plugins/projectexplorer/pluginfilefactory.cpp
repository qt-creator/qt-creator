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

#include "pluginfilefactory.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "iprojectmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/messagemanager.h>

#include <QtCore/QDebug>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

ProjectFileFactory::ProjectFileFactory(IProjectManager *manager)
  : m_mimeTypes(manager->mimeType()),
    m_kind(Constants::FILE_FACTORY_KIND),
    m_manager(manager)
{
}

QStringList ProjectFileFactory::mimeTypes() const
{
    return m_mimeTypes;
}

QString ProjectFileFactory::kind() const
{
    return m_kind;
}

Core::IFile *ProjectFileFactory::open(const QString &fileName)
{
    Core::IFile *fIFace = 0;

    ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();
    if (!pe->openProject(fileName)) {
        Core::ICore::instance()->messageManager()->printToOutputPane(tr("Could not open the following project: '%1'").arg(fileName));
    } else if (pe->session()) {
        SessionManager *session = pe->session();
        if (session->projects().count() == 1)
            fIFace = session->projects().first()->file();
        else if (session->projects().count() > 1)
            fIFace = session->file();  // TODO: Why return session file interface here ???
    }
    return fIFace;
}

QList<ProjectFileFactory *> ProjectFileFactory::createFactories(QString *filterString)
{
    // Register factories for all project managers
    QList<Internal::ProjectFileFactory*> rc;
    QList<IProjectManager*> projectManagers =
        ExtensionSystem::PluginManager::instance()->getObjects<IProjectManager>();

    const QString filterSeparator = QLatin1String(";;");
    filterString->clear();
    foreach (IProjectManager *manager, projectManagers) {
        rc.push_back(new ProjectFileFactory(manager));
        if (!filterString->isEmpty())
            *filterString += filterSeparator;
        const QString mimeType = manager->mimeType();
        const QString pFilterString = Core::ICore::instance()->mimeDatabase()->findByType(mimeType).filterString();
        *filterString += pFilterString;
    }
    return rc;
}
