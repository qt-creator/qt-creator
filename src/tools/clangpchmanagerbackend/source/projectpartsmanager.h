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

#include "clangpchmanagerbackend_global.h"

#include <precompiledheaderstorageinterface.h>
#include <projectpartsmanagerinterface.h>
#include <projectpartsstorageinterface.h>

#include <utils/smallstringvector.h>

namespace ClangBackEnd {

class BuildDependenciesProviderInterface;
class FilePathCachingInterface;
class GeneratedFilesInterface;
class ClangPathWatcherInterface;

inline namespace Pch {

class ProjectPartsManager final : public ProjectPartsManagerInterface
{
public:
    ProjectPartsManager(ProjectPartsStorageInterface &projectPartsStorage,
                        PrecompiledHeaderStorageInterface &precompiledHeaderStorage,
                        BuildDependenciesProviderInterface &buildDependenciesProvider,
                        FilePathCachingInterface &filePathCache,
                        ClangPathWatcherInterface &clangPathwatcher,
                        GeneratedFilesInterface &generatedFiles)
        : m_projectPartsStorage(projectPartsStorage)
        , m_precompiledHeaderStorage(precompiledHeaderStorage)
        , m_buildDependenciesProvider(buildDependenciesProvider)
        , m_filePathCache(filePathCache)
        , m_clangPathwatcher(clangPathwatcher)
        , m_generatedFiles(generatedFiles)
    {
        Q_UNUSED(m_filePathCache)
    }

    UpToDataProjectParts update(ProjectPartContainers &&projectsParts) override;
    void remove(const ProjectPartIds &projectPartIds) override;
    ProjectPartContainers projects(const ProjectPartIds &projectPartIds) const override;
    void updateDeferred(ProjectPartContainers &&system, ProjectPartContainers &&project) override;
    ProjectPartContainers deferredSystemUpdates() override;
    ProjectPartContainers deferredProjectUpdates() override;

    static ProjectPartContainers filterProjectParts(const ProjectPartContainers &newProjectsParts,
                                                    const ProjectPartContainers &oldProjectParts);
    void mergeProjectParts(const ProjectPartContainers &projectsParts);
    const ProjectPartContainers &projectParts() const;
    UpToDataProjectParts checkDependeciesAndTime(ProjectPartContainers &&upToDateProjectParts,
                                                 ProjectPartContainers &&updateSystemProjectParts);

private:
    ProjectPartContainers m_projectParts;
    ProjectPartContainers m_systemDeferredProjectParts;
    ProjectPartContainers m_projectDeferredProjectParts;
    ProjectPartsStorageInterface &m_projectPartsStorage;
    PrecompiledHeaderStorageInterface &m_precompiledHeaderStorage;
    BuildDependenciesProviderInterface &m_buildDependenciesProvider;
    FilePathCachingInterface &m_filePathCache;
    ClangPathWatcherInterface &m_clangPathwatcher;
    GeneratedFilesInterface &m_generatedFiles;
};

} // namespace Pch

} // namespace ClangBackEnd
