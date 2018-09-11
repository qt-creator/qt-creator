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

#include "projectupdater.h"

#include "pchmanagerclient.h"

#include <filepathid.h>
#include <pchmanagerserverinterface.h>
#include <removegeneratedfilesmessage.h>
#include <removeprojectpartsmessage.h>
#include <updategeneratedfilesmessage.h>
#include <updateprojectpartsmessage.h>

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/projectpart.h>

#include <utils/algorithm.h>

#include <algorithm>
#include <functional>

namespace ClangPchManager {

class HeaderAndSources
{
public:
    void reserve(std::size_t size)
    {
        headers.reserve(size);
        sources.reserve(size);
    }

    ClangBackEnd::FilePathIds headers;
    ClangBackEnd::FilePathIds sources;
};

ProjectUpdater::ProjectUpdater(ClangBackEnd::ProjectManagementServerInterface &server,
                               ClangBackEnd::FilePathCachingInterface &filePathCache)
    : m_server(server),
    m_filePathCache(filePathCache)
{
}

void ProjectUpdater::updateProjectParts(const std::vector<CppTools::ProjectPart *> &projectParts)
{
    m_server.updateProjectParts(
                ClangBackEnd::UpdateProjectPartsMessage{toProjectPartContainers(projectParts)});
}

void ProjectUpdater::removeProjectParts(const QStringList &projectPartIds)
{
    Utils::SmallStringVector sortedIds(projectPartIds);
    std::sort(sortedIds.begin(), sortedIds.end());

    m_server.removeProjectParts(ClangBackEnd::RemoveProjectPartsMessage{std::move(sortedIds)});
}

void ProjectUpdater::updateGeneratedFiles(ClangBackEnd::V2::FileContainers &&generatedFiles)
{
    std::sort(generatedFiles.begin(), generatedFiles.end());

    m_generatedFiles.update(generatedFiles);

    m_excludedPaths = createExcludedPaths(m_generatedFiles.fileContainers());

    m_server.updateGeneratedFiles(
                ClangBackEnd::UpdateGeneratedFilesMessage{std::move(generatedFiles)});
}

void ProjectUpdater::removeGeneratedFiles(ClangBackEnd::FilePaths &&filePaths)
{
    m_generatedFiles.remove(filePaths);

    m_excludedPaths = createExcludedPaths(m_generatedFiles.fileContainers());

    m_server.removeGeneratedFiles(
                ClangBackEnd::RemoveGeneratedFilesMessage{std::move(filePaths)});
}

void ProjectUpdater::setExcludedPaths(ClangBackEnd::FilePaths &&excludedPaths)
{
    m_excludedPaths = std::move(excludedPaths);
}

const ClangBackEnd::FilePaths &ProjectUpdater::excludedPaths() const
{
    return m_excludedPaths;
}

const ClangBackEnd::GeneratedFiles &ProjectUpdater::generatedFiles() const
{
    return m_generatedFiles;
}

void ProjectUpdater::addToHeaderAndSources(HeaderAndSources &headerAndSources,
                                           const CppTools::ProjectFile &projectFile) const
{
    using ClangBackEnd::FilePathView;

    Utils::PathString path = projectFile.path;
    bool exclude = std::binary_search(m_excludedPaths.begin(), m_excludedPaths.end(), path);

    if (!exclude) {
        ClangBackEnd::FilePathId filePathId = m_filePathCache.filePathId(FilePathView(path));
        if (projectFile.isSource())
            headerAndSources.sources.push_back(filePathId);
        else if (projectFile.isHeader())
            headerAndSources.headers.push_back(filePathId);
    }
}

HeaderAndSources ProjectUpdater::headerAndSourcesFromProjectPart(
        CppTools::ProjectPart *projectPart) const
{
    HeaderAndSources headerAndSources;
    headerAndSources.reserve(std::size_t(projectPart->files.size()) * 3 / 2);

    for (const CppTools::ProjectFile &projectFile : projectPart->files)
        addToHeaderAndSources(headerAndSources, projectFile);

    std::sort(headerAndSources.sources.begin(), headerAndSources.sources.end());
    std::sort(headerAndSources.headers.begin(), headerAndSources.headers.end());

    return headerAndSources;
}

QStringList ProjectUpdater::compilerArguments(CppTools::ProjectPart *projectPart)
{
    using CppTools::CompilerOptionsBuilder;
    CompilerOptionsBuilder builder(*projectPart, CppTools::UseSystemHeader::Yes);
    return builder.build(CppTools::ProjectFile::CXXHeader, CompilerOptionsBuilder::PchUsage::None);
}

ClangBackEnd::CompilerMacros ProjectUpdater::createCompilerMacros(const ProjectExplorer::Macros &projectMacros)
{
    auto macros =  Utils::transform<ClangBackEnd::CompilerMacros>(projectMacros,
                                                                  [] (const ProjectExplorer::Macro &macro) {
        return ClangBackEnd::CompilerMacro{macro.key, macro.value};
    });

    std::sort(macros.begin(), macros.end());

    return macros;
}

Utils::SmallStringVector ProjectUpdater::createIncludeSearchPaths(
        const ProjectExplorer::HeaderPaths &projectPartHeaderPaths)
{
    Utils::SmallStringVector includePaths;

    for (const ProjectExplorer::HeaderPath &projectPartHeaderPath : projectPartHeaderPaths) {
        if (!projectPartHeaderPath.path.isEmpty())
            includePaths.emplace_back(projectPartHeaderPath.path);
    }

    std::sort(includePaths.begin(), includePaths.end());

    return includePaths;
}

ClangBackEnd::V2::ProjectPartContainer ProjectUpdater::toProjectPartContainer(
        CppTools::ProjectPart *projectPart) const
{

    QStringList arguments = compilerArguments(projectPart);

    HeaderAndSources headerAndSources = headerAndSourcesFromProjectPart(projectPart);

    return ClangBackEnd::V2::ProjectPartContainer(projectPart->id(),
                                                  Utils::SmallStringVector(arguments),
                                                  createCompilerMacros(projectPart->projectMacros),
                                                  createIncludeSearchPaths(projectPart->headerPaths),
                                                  std::move(headerAndSources.headers),
                                                  std::move(headerAndSources.sources));
}

std::vector<ClangBackEnd::V2::ProjectPartContainer> ProjectUpdater::toProjectPartContainers(
        std::vector<CppTools::ProjectPart *> projectParts) const
{
    using namespace std::placeholders;

    std::vector<ClangBackEnd::V2::ProjectPartContainer> projectPartContainers;
    projectPartContainers.reserve(projectParts.size());

    std::transform(projectParts.begin(),
                   projectParts.end(),
                   std::back_inserter(projectPartContainers),
                   std::bind(&ProjectUpdater::toProjectPartContainer, this, _1));

    std::sort(projectPartContainers.begin(), projectPartContainers.end());

    return projectPartContainers;
}

ClangBackEnd::FilePaths ProjectUpdater::createExcludedPaths(
        const ClangBackEnd::V2::FileContainers &generatedFiles)
{
    ClangBackEnd::FilePaths excludedPaths;
    excludedPaths.reserve(generatedFiles.size());

    auto convertToPath = [] (const ClangBackEnd::V2::FileContainer &fileContainer) {
        return fileContainer.filePath;
    };

    std::transform(generatedFiles.begin(),
                   generatedFiles.end(),
                   std::back_inserter(excludedPaths),
                   convertToPath);

    std::sort(excludedPaths.begin(), excludedPaths.end());

    return excludedPaths;
}

} // namespace ClangPchManager
