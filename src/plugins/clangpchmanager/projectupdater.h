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

#include "clangpchmanager_global.h"

#include <compilermacro.h>
#include <filecontainerv2.h>
#include <filepathcachinginterface.h>
#include <generatedfiles.h>
#include <includesearchpath.h>
#include <projectpartcontainer.h>
#include <projectpartsstorageinterface.h>
#include <projectpartstoragestructs.h>
#include <stringcache.h>

#include <projectexplorer/headerpath.h>

#include <utils/environmentfwd.h>

namespace ProjectExplorer {
class Macro;
using Macros = QVector<Macro>;
}

namespace CppTools {
class ProjectPart;
class ProjectFile;
}

namespace ClangBackEnd {
class ProjectManagementServerInterface;
}

QT_FORWARD_DECLARE_CLASS(QStringList)

#include <vector>

namespace ClangPchManager {

class HeaderAndSources;
class PchManagerClient;
class ClangIndexingSettingsManager;
class ClangIndexingProjectSettings;

class CLANGPCHMANAGER_EXPORT ProjectUpdater
{
    using StringCache = ClangBackEnd::StringCache<Utils::PathString,
                                                  Utils::SmallStringView,
                                                  ClangBackEnd::ProjectPartId,
                                                  ClangBackEnd::NonLockingMutex,
                                                  decltype(&Utils::reverseCompare),
                                                  Utils::reverseCompare,
                                                  ClangBackEnd::Internal::ProjectPartNameId>;

public:
    struct SystemAndProjectIncludeSearchPaths
    {
        ClangBackEnd::IncludeSearchPaths system;
        ClangBackEnd::IncludeSearchPaths project;
    };

    ProjectUpdater(ClangBackEnd::ProjectManagementServerInterface &server,
                   ClangBackEnd::FilePathCachingInterface &filePathCache,
                   ClangBackEnd::ProjectPartsStorageInterface &projectPartsStorage,
                   ClangIndexingSettingsManager &settingsManager)
        : m_filePathCache(filePathCache)
        , m_server(server)
        , m_projectPartsStorage(projectPartsStorage)
        , m_settingsManager(settingsManager)
    {
        m_projectPartIdCache.populate(m_projectPartsStorage.fetchAllProjectPartNamesAndIds());
    }

    void updateProjectParts(const std::vector<CppTools::ProjectPart *> &projectParts,
                            Utils::SmallStringVector &&toolChainArguments);
    void removeProjectParts(ClangBackEnd::ProjectPartIds projectPartIds);

    void updateGeneratedFiles(ClangBackEnd::V2::FileContainers &&generatedFiles);
    void removeGeneratedFiles(ClangBackEnd::FilePaths &&filePaths);

    void setExcludedPaths(ClangBackEnd::FilePaths &&excludedPaths);
    const ClangBackEnd::FilePaths &excludedPaths() const;

    const ClangBackEnd::GeneratedFiles &generatedFiles() const;

    HeaderAndSources headerAndSourcesFromProjectPart(CppTools::ProjectPart *projectPart) const;
    ClangBackEnd::ProjectPartContainer toProjectPartContainer(
            CppTools::ProjectPart *projectPart) const;
    ClangBackEnd::ProjectPartContainers toProjectPartContainers(
            std::vector<CppTools::ProjectPart *> projectParts) const;

    void addToHeaderAndSources(HeaderAndSources &headerAndSources,
                               const CppTools::ProjectFile &projectFile) const;
    static QStringList toolChainArguments(CppTools::ProjectPart *projectPart);
    static ClangBackEnd::CompilerMacros createCompilerMacros(const ProjectExplorer::Macros &projectMacros,
                                                             Utils::NameValueItems &&settingsMacros);
    static SystemAndProjectIncludeSearchPaths createIncludeSearchPaths(
        const CppTools::ProjectPart &projectPart);
    static ClangBackEnd::FilePaths createExcludedPaths(
            const ClangBackEnd::V2::FileContainers &generatedFiles);

    QString fetchProjectPartName(ClangBackEnd::ProjectPartId projectPartId) const;

    ClangBackEnd::ProjectPartIds toProjectPartIds(const QStringList &projectPartNames) const;

    void addProjectFilesToFilePathCache(const std::vector<CppTools::ProjectPart *> &projectParts);
    void fetchProjectPartIds(const std::vector<CppTools::ProjectPart *> &projectParts);

protected:
    ClangBackEnd::FilePathCachingInterface &m_filePathCache;

private:
    ClangBackEnd::GeneratedFiles m_generatedFiles;
    ClangBackEnd::FilePaths m_excludedPaths;
    ClangBackEnd::ProjectManagementServerInterface &m_server;
    ClangBackEnd::ProjectPartsStorageInterface &m_projectPartsStorage;
    ClangIndexingSettingsManager &m_settingsManager;
    mutable StringCache m_projectPartIdCache;
};

} // namespace ClangPchManager
