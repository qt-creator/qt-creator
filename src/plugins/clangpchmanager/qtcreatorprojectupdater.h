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
#include <projectexplorer/extracompiler.h>

#include <filecontainerv2.h>

#include <utils/algorithm.h>

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
CLANGPCHMANAGER_EXPORT std::vector<ClangBackEnd::V2::FileContainer> createGeneratedFiles(
    ClangBackEnd::FilePathCachingInterface &filePathCache);
CLANGPCHMANAGER_EXPORT std::vector<CppTools::ProjectPart*> createProjectParts(ProjectExplorer::Project *project);
}

template<typename ProjectUpdaterType>
class QtCreatorProjectUpdater : public ProjectUpdaterType,
                                public ProjectExplorer::ExtraCompilerFactoryObserver
{
public:
    template<typename ClientType>
    QtCreatorProjectUpdater(ClangBackEnd::ProjectManagementServerInterface &server,
                            ClientType &client,
                            ClangBackEnd::FilePathCachingInterface &filePathCache,
                            ClangBackEnd::ProjectPartsStorageInterface &projectPartsStorage,
                            ClangIndexingSettingsManager &settingsManager)
        : ProjectUpdaterType(server, client, filePathCache, projectPartsStorage, settingsManager)
    {
        connectToCppModelManager();
    }

    QtCreatorProjectUpdater(ClangBackEnd::ProjectManagementServerInterface &server,
                            ClangBackEnd::FilePathCachingInterface &filePathCache)
        : ProjectUpdaterType(server, filePathCache)
    {
        connectToCppModelManager();
    }

    void projectPartsUpdated(ProjectExplorer::Project *project)
    {
        ProjectUpdaterType::updateProjectParts(Internal::createProjectParts(project), {}); // TODO add support for toolchainarguments
    }

    void projectPartsRemoved(const QStringList &projectPartIds)
    {    
        ProjectUpdaterType::removeProjectParts(projectPartIds);
    }

    void abstractEditorUpdated(const QString &qFilePath, const QByteArray &contents)
    {
        ClangBackEnd::FilePath filePath{qFilePath};
        ClangBackEnd::FilePathId filePathId = ProjectUpdaterType::m_filePathCache.filePathId(
            filePath);

        ProjectUpdaterType::updateGeneratedFiles({{std::move(filePath), filePathId, contents}});
    }

    void abstractEditorRemoved(const QString &filePath)
    {
        ProjectUpdaterType::removeGeneratedFiles({ClangBackEnd::FilePath{filePath}});
    }

protected:
    void newExtraCompiler(const ProjectExplorer::Project *,
                          const Utils::FilePath &,
                          const Utils::FilePaths &targets) override
    {
        auto filePaths = Utils::transform<ClangBackEnd::FilePaths>(targets,
                                                                   [](const Utils::FilePath &filePath) {
                                                                       return ClangBackEnd::FilePath{
                                                                           filePath.toString()};
                                                                   });

        ProjectUpdater::m_filePathCache.addFilePaths(filePaths);

        for (const Utils::FilePath &target : targets)
            abstractEditorUpdated(target.toString(), {});
    }

private:
    void connectToCppModelManager()
    {
        ProjectUpdaterType::updateGeneratedFiles(
            Internal::createGeneratedFiles(ProjectUpdaterType::m_filePathCache));

        QObject::connect(Internal::cppModelManager(),
                         &CppTools::CppModelManager::projectPartsUpdated,
                         [&] (ProjectExplorer::Project *project) { projectPartsUpdated(project); });
        QObject::connect(Internal::cppModelManager(),
                         &CppTools::CppModelManager::projectPartsRemoved,
                         [&] (const QStringList &projectPartIds) { projectPartsRemoved(projectPartIds); });
        QObject::connect(Internal::cppModelManager(),
                         &CppTools::CppModelManager::abstractEditorSupportContentsUpdated,
                         [&](const QString &filePath,
                             const QString &,
                             const QByteArray &contents) {
                             abstractEditorUpdated(filePath, contents);
                         });
        QObject::connect(Internal::cppModelManager(),
                         &CppTools::CppModelManager::abstractEditorSupportRemoved,
                         [&] (const QString &filePath) { abstractEditorRemoved(filePath); });
    }
};

} // namespace ClangPchManager
