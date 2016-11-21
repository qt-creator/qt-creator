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

#include "clangquerycurrentfilefindfilter.h"

#include "projectpartutilities.h"
#include "refactoringclient.h"
#include "refactoringcompileroptionsbuilder.h"
#include "searchinterface.h"

#include <refactoringserverinterface.h>
#include <requestsourcerangesanddiagnosticsforquerymessage.h>

namespace ClangRefactoring {

ClangQueryCurrentFileFindFilter::ClangQueryCurrentFileFindFilter(
        ClangBackEnd::RefactoringServerInterface &server,
        SearchInterface &searchInterface,
        RefactoringClient &refactoringClient)
    : server(server),
      searchInterface(searchInterface),
      refactoringClient(refactoringClient)
{
}

QString ClangQueryCurrentFileFindFilter::id() const
{
    return QStringLiteral("Clang Query Current File");
}

QString ClangQueryCurrentFileFindFilter::displayName() const
{
    return tr("Clang Query Current File");
}

bool ClangQueryCurrentFileFindFilter::isEnabled() const
{
    return true;
}

void ClangQueryCurrentFileFindFilter::findAll(const QString &queryText, Core::FindFlags)
{
    searchHandle = searchInterface.startNewSearch(tr("Clang Query"), queryText);

    refactoringClient.setSearchHandle(searchHandle.get());

    server.requestSourceRangesAndDiagnosticsForQueryMessage(createMessage(queryText));
}

Core::FindFlags ClangQueryCurrentFileFindFilter::supportedFindFlags() const
{
    return 0;
}

void ClangQueryCurrentFileFindFilter::setCurrentDocumentFilePath(const QString &filePath)
{
    currentDocumentFilePath = filePath;
}

void ClangQueryCurrentFileFindFilter::setUnsavedDocumentContent(const QString &unsavedContent)
{
    unsavedDocumentContent = unsavedContent;
}

void ClangQueryCurrentFileFindFilter::setProjectPart(const CppTools::ProjectPart::Ptr &projectPart)
{
    this->projectPart = projectPart;
}

void ClangQueryCurrentFileFindFilter::setUsable(bool isUsable)
{
    server.setUsable(isUsable);
}

bool ClangQueryCurrentFileFindFilter::isUsable() const
{
    return server.isUsable();
}

ClangBackEnd::RequestSourceRangesAndDiagnosticsForQueryMessage
ClangQueryCurrentFileFindFilter::createMessage(const QString &queryText) const
{
    std::vector<ClangBackEnd::V2::FileContainer> fileContainers;
    fileContainers.emplace_back(ClangBackEnd::FilePath(currentDocumentFilePath),
                                unsavedDocumentContent,
                                createCommandLine());

    return ClangBackEnd::RequestSourceRangesAndDiagnosticsForQueryMessage(
                Utils::SmallString(queryText),
                std::move(fileContainers));
}

Utils::SmallStringVector ClangQueryCurrentFileFindFilter::createCommandLine() const
{
    using ClangRefactoring::RefactoringCompilerOptionsBuilder;

    auto commandLine = RefactoringCompilerOptionsBuilder::build(
                projectPart.data(),
                fileKindInProjectPart(projectPart.data(), currentDocumentFilePath),
                RefactoringCompilerOptionsBuilder::PchUsage::None);

    commandLine.push_back(currentDocumentFilePath);

    return commandLine;
}

} // namespace ClangRefactoring
