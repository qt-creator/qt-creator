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

#include "pchmanagerprojectupdater.h"

#include <cpptools/cppmodelmanager.h>

#include <filecontainerv2.h>

#include <QObject>

namespace ProjectExplorer {
class Project;
}

namespace CppTools {
class CppModelManager;
}

namespace ClangPchManager {

namespace Internal {
CLANGPCHMANAGER_EXPORT CppTools::CppModelManager *cppModelManager();
CLANGPCHMANAGER_EXPORT std::vector<ClangBackEnd::V2::FileContainer> createGeneratedFiles();
CLANGPCHMANAGER_EXPORT std::vector<CppTools::ProjectPart*> createProjectParts(ProjectExplorer::Project *project);
}

template <typename ProjectUpdaterType>
class QtCreatorProjectUpdater : public ProjectUpdaterType
{
public:
    template <typename ClientType>
    QtCreatorProjectUpdater(ClangBackEnd::ProjectManagementServerInterface &server,
                            ClientType &client)
        : ProjectUpdaterType(server, client)
    {
        connectToCppModelManager();
    }

    QtCreatorProjectUpdater(ClangBackEnd::ProjectManagementServerInterface &server)
        : ProjectUpdaterType(server)
    {
        connectToCppModelManager();
    }

    void projectPartsUpdated(ProjectExplorer::Project *project)
    {
        ProjectUpdaterType::updateProjectParts(Internal::createProjectParts(project),
                                               Internal::createGeneratedFiles());
    }

    void projectPartsRemoved(const QStringList &projectPartIds)
    {
        ProjectUpdaterType::removeProjectParts(projectPartIds);
    }

private:
    void connectToCppModelManager()
    {
        QObject::connect(Internal::cppModelManager(),
                         &CppTools::CppModelManager::projectPartsUpdated,
                         [&] (ProjectExplorer::Project *project) { projectPartsUpdated(project); });
        QObject::connect(Internal::cppModelManager(),
                         &CppTools::CppModelManager::projectPartsRemoved,
                         [&] (const QStringList &projectPartIds) { projectPartsRemoved(projectPartIds); });
    }
};

} // namespace ClangPchManager
