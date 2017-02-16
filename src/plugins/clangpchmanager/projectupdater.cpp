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

#include <pchmanagerserverinterface.h>
#include <removepchprojectpartsmessage.h>
#include <updatepchprojectpartsmessage.h>

#include <cpptools/clangcompileroptionsbuilder.h>
#include <cpptools/projectpart.h>

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

    Utils::PathStringVector headers;
    Utils::PathStringVector sources;
};

ProjectUpdater::ProjectUpdater(ClangBackEnd::PchManagerServerInterface &server,
                               PchManagerClient &client)
    : m_server(server),
      m_client(client)
{
}

void ProjectUpdater::updateProjectParts(const std::vector<CppTools::ProjectPart *> &projectParts,
                                        ClangBackEnd::V2::FileContainers &&generatedFiles)
{
    m_excludedPaths = createExcludedPaths(generatedFiles);

    ClangBackEnd::UpdatePchProjectPartsMessage message{toProjectPartContainers(projectParts),
                                                       std::move(generatedFiles)};

    m_server.updatePchProjectParts(std::move(message));
}

void ProjectUpdater::removeProjectParts(const QStringList &projectPartIds)
{
    ClangBackEnd::RemovePchProjectPartsMessage message{Utils::SmallStringVector(projectPartIds)};

    m_server.removePchProjectParts(std::move(message));

    for (const QString &projectPartiId : projectPartIds)
        m_client.precompiledHeaderRemoved(projectPartiId);
}

void ProjectUpdater::setExcludedPaths(Utils::PathStringVector &&excludedPaths)
{
    m_excludedPaths = excludedPaths;
}

void ProjectUpdater::addToHeaderAndSources(HeaderAndSources &headerAndSources,
                                           const CppTools::ProjectFile &projectFile) const
{
    Utils::PathString path = projectFile.path;
    bool exclude = std::binary_search(m_excludedPaths.begin(), m_excludedPaths.end(), path);

    if (!exclude) {
        if (projectFile.isSource())
            headerAndSources.sources.push_back(path);
        else if (projectFile.isHeader())
            headerAndSources.headers.push_back(path);
    }
}

HeaderAndSources ProjectUpdater::headerAndSourcesFromProjectPart(
        CppTools::ProjectPart *projectPart) const
{
    HeaderAndSources headerAndSources;
    headerAndSources.reserve(std::size_t(projectPart->files.size()) * 3 / 2);

    for (const CppTools::ProjectFile &projectFile : projectPart->files)
        addToHeaderAndSources(headerAndSources, projectFile);

    return headerAndSources;
}

QStringList ProjectUpdater::compilerArguments(CppTools::ProjectPart *projectPart)
{
    using CppTools::ClangCompilerOptionsBuilder;

        ClangCompilerOptionsBuilder builder(*projectPart, CLANG_VERSION, CLANG_RESOURCE_DIR);

        builder.addWordWidth();
        builder.addTargetTriple();
        builder.addLanguageOption(CppTools::ProjectFile::CXXHeader);
        builder.addOptionsForLanguage(/*checkForBorlandExtensions*/ true);
        builder.enableExceptions();

        builder.addDefineToAvoidIncludingGccOrMinGwIntrinsics();
        builder.addDefineFloat128ForMingw();
        builder.addToolchainAndProjectDefines();
        builder.undefineCppLanguageFeatureMacrosForMsvc2015();

        builder.addPredefinedMacrosAndHeaderPathsOptions();
        builder.addWrappedQtHeadersIncludePath();
        builder.addPrecompiledHeaderOptions(ClangCompilerOptionsBuilder::PchUsage::None);
        builder.addHeaderPathOptions();
        builder.addProjectConfigFileInclude();

        builder.addMsvcCompatibilityVersion();

        builder.add("-fmessage-length=0");
        builder.add("-fmacro-backtrace-limit=0");
        builder.add("-w");
        builder.add("-ferror-limit=100000");

        return builder.options();
}

ClangBackEnd::V2::ProjectPartContainer ProjectUpdater::toProjectPartContainer(
        CppTools::ProjectPart *projectPart) const
{

    QStringList arguments = compilerArguments(projectPart);

    HeaderAndSources headerAndSources = headerAndSourcesFromProjectPart(projectPart);

    return ClangBackEnd::V2::ProjectPartContainer(projectPart->displayName,
                                                  Utils::SmallStringVector(arguments),
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

    return projectPartContainers;
}

Utils::PathStringVector ProjectUpdater::createExcludedPaths(
        const ClangBackEnd::V2::FileContainers &generatedFiles)
{
    Utils::PathStringVector excludedPaths;
    excludedPaths.reserve(generatedFiles.size());

    auto convertToPath = [] (const ClangBackEnd::V2::FileContainer &fileContainer) {
        return fileContainer.filePath().path();
    };

    std::transform(generatedFiles.begin(),
                   generatedFiles.end(),
                   std::back_inserter(excludedPaths),
                   convertToPath);

    std::sort(excludedPaths.begin(), excludedPaths.end());

    return excludedPaths;
}

} // namespace ClangPchManager
