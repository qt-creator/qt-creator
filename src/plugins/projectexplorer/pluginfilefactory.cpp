/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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

ProjectFileFactory::ProjectFileFactory(const Core::ICore* core, IProjectManager *manager) :
    m_mimeTypes(manager->mimeType()),
    m_kind(Constants::FILE_FACTORY_KIND),
    m_core(core),
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
        m_core->messageManager()->printToOutputPane(tr("Could not open the following project: '%1'").arg(fileName));
    } else if (pe->session()) {
        SessionManager *session = pe->session();
        if (session->projects().count() == 1)
            fIFace = session->projects().first()->file();
        else if (session->projects().count() > 1)
            fIFace = session->file();  // TODO: Why return session file interface here ???
    }
    return fIFace;
}

QList<ProjectFileFactory*> ProjectFileFactory::createFactories(const Core::ICore* core,
                                                               QString *filterString)
{
    // Register factories for all project managers
    QList<Internal::ProjectFileFactory*> rc;
    QList<IProjectManager*> projectManagers = core->pluginManager()->getObjects<IProjectManager>();

    const QString filterSeparator = QLatin1String(";;");
    filterString->clear();
    foreach(IProjectManager *manager, projectManagers) {
        rc.push_back(new ProjectFileFactory(core, manager));
        if (!filterString->isEmpty())
            *filterString += filterSeparator;
        const QString mimeType = manager->mimeType();
        const QString pFilterString = core->mimeDatabase()->findByType(mimeType).filterString();
        *filterString += pFilterString;
    }
    return rc;
}
