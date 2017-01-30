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

namespace ClangPchManager {

ProjectUpdater::ProjectUpdater(ClangBackEnd::PchManagerServerInterface &server,
                               PchManagerClient &client)
    : m_server(server),
      m_client(client)
{
}

void ProjectUpdater::updateProjectParts(const std::vector<CppTools::ProjectPart *> &projectParts)
{
    ClangBackEnd::UpdatePchProjectPartsMessage message{toProjectPartContainers(projectParts)};

    m_server.updatePchProjectParts(std::move(message));
}

void ProjectUpdater::removeProjectParts(const QStringList &projectPartIds)
{
    ClangBackEnd::RemovePchProjectPartsMessage message{Utils::SmallStringVector(projectPartIds)};

    m_server.removePchProjectParts(std::move(message));

    for (const QString &projectPartiId : projectPartIds)
        m_client.precompiledHeaderRemoved(projectPartiId);
}

namespace {
class HeaderAndSources
{
public:
    void reserve(std::size_t size)
    {
        headers.reserve(size);
        sources.reserve(size);
    }

    Utils::SmallStringVector headers;
    Utils::SmallStringVector sources;
};

HeaderAndSources headerAndSourcesFromProjectPart(CppTools::ProjectPart *projectPart)
{
    HeaderAndSources headerAndSources;
    headerAndSources.reserve(std::size_t(projectPart->files.size()) * 3 / 2);

    for (const CppTools::ProjectFile &projectFile : projectPart->files) {
        if (projectFile.isSource())
            headerAndSources.sources.push_back(projectFile.path);
        else if (projectFile.isHeader())
             headerAndSources.headers.push_back(projectFile.path);
    }

    return headerAndSources;
}
}

ClangBackEnd::V2::ProjectPartContainer ProjectUpdater::toProjectPartContainer(CppTools::ProjectPart *projectPart)
{
    using CppTools::ClangCompilerOptionsBuilder;

    QStringList arguments = ClangCompilerOptionsBuilder::build(
                projectPart,
                CppTools::ProjectFile::CXXHeader,
                ClangCompilerOptionsBuilder::PchUsage::None,
                CLANG_VERSION,
                CLANG_RESOURCE_DIR);

    HeaderAndSources headerAndSources = headerAndSourcesFromProjectPart(projectPart);

    return ClangBackEnd::V2::ProjectPartContainer(projectPart->displayName,
                                                  Utils::SmallStringVector(arguments),
                                                  std::move(headerAndSources.headers),
                                                  std::move(headerAndSources.sources));
}

std::vector<ClangBackEnd::V2::ProjectPartContainer> ProjectUpdater::toProjectPartContainers(
        std::vector<CppTools::ProjectPart *> projectParts)
{
    std::vector<ClangBackEnd::V2::ProjectPartContainer> projectPartContainers;
    projectPartContainers.reserve(projectParts.size());

    std::transform(projectParts.begin(),
                   projectParts.end(),
                   std::back_inserter(projectPartContainers),
                   ProjectUpdater::toProjectPartContainer);

    return projectPartContainers;
}

} // namespace ClangPchManager
