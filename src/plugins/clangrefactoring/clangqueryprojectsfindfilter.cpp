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

#include "clangqueryprojectsfindfilter.h"

#include "projectpartutilities.h"
#include "refactoringclient.h"
#include "searchinterface.h"

#include <refactoringserverinterface.h>
#include <requestsourcerangesanddiagnosticsforquerymessage.h>

#include <cpptools/clangcompileroptionsbuilder.h>

namespace ClangRefactoring {

ClangQueryProjectsFindFilter::ClangQueryProjectsFindFilter(
        ClangBackEnd::RefactoringServerInterface &server,
        SearchInterface &searchInterface,
        RefactoringClient &refactoringClient)
    : server(server),
      searchInterface(searchInterface),
      refactoringClient(refactoringClient)
{
}

QString ClangQueryProjectsFindFilter::id() const
{
    return QStringLiteral("Clang Query Project");
}

QString ClangQueryProjectsFindFilter::displayName() const
{
    return tr("Clang Query Project");
}

bool ClangQueryProjectsFindFilter::isEnabled() const
{
    return true;
}

void ClangQueryProjectsFindFilter::findAll(const QString &queryText, Core::FindFlags)
{
    searchHandle = searchInterface.startNewSearch(tr("Clang Query"), queryText);

    searchHandle->setRefactoringServer(&server);

    refactoringClient.setSearchHandle(searchHandle.get());

    auto message = createMessage(queryText);

    refactoringClient.setExpectedResultCount(message.sources().size());

    server.requestSourceRangesAndDiagnosticsForQueryMessage(std::move(message));
}

Core::FindFlags ClangQueryProjectsFindFilter::supportedFindFlags() const
{
    return 0;
}

void ClangQueryProjectsFindFilter::setProjectParts(const std::vector<CppTools::ProjectPart::Ptr> &projectParts)
{
    this->projectParts = projectParts;
}

bool ClangQueryProjectsFindFilter::isUsable() const
{
    return server.isUsable();
}

void ClangQueryProjectsFindFilter::setUsable(bool isUsable)
{
    server.setUsable(isUsable);
}

SearchHandle *ClangQueryProjectsFindFilter::searchHandleForTestOnly() const
{
    return searchHandle.get();
}

void ClangQueryProjectsFindFilter::setUnsavedContent(
        std::vector<ClangBackEnd::V2::FileContainer> &&unsavedContent)
{
    this->unsavedContent = std::move(unsavedContent);
}

namespace {

Utils::SmallStringVector createCommandLine(CppTools::ProjectPart *projectPart,
                                           const QString &documentFilePath,
                                           CppTools::ProjectFile::Kind fileKind)
{
    using CppTools::ClangCompilerOptionsBuilder;

    Utils::SmallStringVector commandLine{ClangCompilerOptionsBuilder::build(
                    projectPart,
                    fileKind,
                    CppTools::CompilerOptionsBuilder::PchUsage::None,
                    CLANG_VERSION,
                    CLANG_RESOURCE_DIR)};

    commandLine.push_back(documentFilePath);

    return commandLine;
}

bool unsavedContentContains(const ClangBackEnd::FilePath &sourceFilePath,
                            const std::vector<ClangBackEnd::V2::FileContainer> &unsavedContent)
{
    auto compare = [&] (const ClangBackEnd::V2::FileContainer &unsavedEntry) {
        return unsavedEntry.filePath() == sourceFilePath;
    };

    auto found = std::find_if(unsavedContent.begin(), unsavedContent.end(), compare);

    return found != unsavedContent.end();
}

void appendSource(std::vector<ClangBackEnd::V2::FileContainer> &sources,
                  const CppTools::ProjectPart::Ptr &projectPart,
                  const CppTools::ProjectFile &projectFile,
                  const std::vector<ClangBackEnd::V2::FileContainer> &unsavedContent)
{
    ClangBackEnd::FilePath sourceFilePath(projectFile.path);

    if (!unsavedContentContains(sourceFilePath, unsavedContent)) {
        sources.emplace_back(ClangBackEnd::FilePath(projectFile.path),
                             "",
                             createCommandLine(projectPart.data(),
                                               projectFile.path,
                                               projectFile.kind));
    }
}

std::vector<ClangBackEnd::V2::FileContainer>
createSources(const std::vector<CppTools::ProjectPart::Ptr> &projectParts,
              const std::vector<ClangBackEnd::V2::FileContainer> &unsavedContent)
{
    std::vector<ClangBackEnd::V2::FileContainer> sources;

    for (const CppTools::ProjectPart::Ptr &projectPart : projectParts) {
        for (const CppTools::ProjectFile &projectFile : projectPart->files)
            appendSource(sources, projectPart, projectFile, unsavedContent);
    }

    return sources;
}

}

ClangBackEnd::RequestSourceRangesAndDiagnosticsForQueryMessage ClangQueryProjectsFindFilter::createMessage(const QString &queryText) const
{
    return ClangBackEnd::RequestSourceRangesAndDiagnosticsForQueryMessage(
                Utils::SmallString(queryText),
                createSources(projectParts, unsavedContent),
                Utils::clone(unsavedContent));
}


} // namespace ClangRefactoring
