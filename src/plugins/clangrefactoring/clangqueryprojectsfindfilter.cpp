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
#include <clangrefactoringservermessages.h>

#include <cpptools/compileroptionsbuilder.h>

#include <QPointer>

namespace ClangRefactoring {

ClangQueryProjectsFindFilter::ClangQueryProjectsFindFilter(
        ClangBackEnd::RefactoringServerInterface &server,
        SearchInterface &searchInterface,
        RefactoringClient &refactoringClient)
    : m_server(server),
      m_searchInterface(searchInterface),
      m_refactoringClient(refactoringClient)
{
    temporaryFile.open();
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

namespace {
Utils::SmallString toNative(const QString &path)
{
    Utils::SmallString nativePath = path;

#ifdef Q_OS_WIN
    nativePath.replace('/', '\\');
#endif

    return nativePath;
}
}

void ClangQueryProjectsFindFilter::requestSourceRangesAndDiagnostics(const QString &queryText,
                                                                     const QString &exampleContent)
{
    const QString filePath = temporaryFile.fileName();

    ClangBackEnd::RequestSourceRangesAndDiagnosticsForQueryMessage message(queryText,
                                                                           {ClangBackEnd::FilePath(filePath),
                                                                            exampleContent,
                                                                            {"cc",  "-std=c++1z", toNative(filePath)}});

    m_server.requestSourceRangesAndDiagnosticsForQueryMessage(std::move(message));
}

SearchHandle *ClangQueryProjectsFindFilter::find(const QString &queryText)
{
    m_searchHandle = m_searchInterface.startNewSearch(tr("Clang Query"), queryText);

    m_searchHandle->setRefactoringServer(&m_server);

    m_refactoringClient.setSearchHandle(m_searchHandle.get());

    auto message = createMessage(queryText);

    m_refactoringClient.setExpectedResultCount(uint(message.sources().size()));

    m_server.requestSourceRangesForQueryMessage(std::move(message));

    return m_searchHandle.get();
}

void ClangQueryProjectsFindFilter::findAll(const QString &, Core::FindFlags)
{
    find(queryText());
}

bool ClangQueryProjectsFindFilter::showSearchTermInput() const
{
    return false;
}

Core::FindFlags ClangQueryProjectsFindFilter::supportedFindFlags() const
{
    return 0;
}

void ClangQueryProjectsFindFilter::setProjectParts(const std::vector<CppTools::ProjectPart::Ptr> &projectParts)
{
    this->m_projectParts = projectParts;
}

bool ClangQueryProjectsFindFilter::isAvailable() const
{
    return m_server.isAvailable();
}

void ClangQueryProjectsFindFilter::setAvailable(bool isAvailable)
{
    m_server.setAvailable(isAvailable);
}

SearchHandle *ClangQueryProjectsFindFilter::searchHandleForTestOnly() const
{
    return m_searchHandle.get();
}

void ClangQueryProjectsFindFilter::setUnsavedContent(
        std::vector<ClangBackEnd::V2::FileContainer> &&unsavedContent)
{
    this->m_unsavedContent = std::move(unsavedContent);
}

Utils::SmallStringVector ClangQueryProjectsFindFilter::compilerArguments(CppTools::ProjectPart *projectPart,
                                                                         CppTools::ProjectFile::Kind fileKind)
{
    using CppTools::CompilerOptionsBuilder;

    CompilerOptionsBuilder builder(*projectPart, CLANG_VERSION, CLANG_RESOURCE_DIR);

    return Utils::SmallStringVector(builder.build(fileKind,
                                                  CompilerOptionsBuilder::PchUsage::None));
}

QWidget *ClangQueryProjectsFindFilter::widget() const
{
    return nullptr;
}

namespace {

Utils::SmallStringVector createCommandLine(CppTools::ProjectPart *projectPart,
                                           const QString &documentFilePath,
                                           CppTools::ProjectFile::Kind fileKind)
{
    using CppTools::CompilerOptionsBuilder;

    Utils::SmallStringVector commandLine = ClangQueryProjectsFindFilter::compilerArguments(projectPart, fileKind);

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

bool isCHeader(CppTools::ProjectFile::Kind kind)
{
    return kind == CppTools::ProjectFile::CHeader;
}

void appendSource(std::vector<ClangBackEnd::V2::FileContainer> &sources,
                  const CppTools::ProjectPart::Ptr &projectPart,
                  const CppTools::ProjectFile &projectFile,
                  const std::vector<ClangBackEnd::V2::FileContainer> &unsavedContent)
{
    ClangBackEnd::FilePath sourceFilePath(projectFile.path);

    if (!unsavedContentContains(sourceFilePath, unsavedContent) && !isCHeader(projectFile.kind)) {
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

ClangBackEnd::RequestSourceRangesForQueryMessage ClangQueryProjectsFindFilter::createMessage(const QString &queryText) const
{
    return ClangBackEnd::RequestSourceRangesForQueryMessage(
                Utils::SmallString(queryText),
                createSources(m_projectParts, m_unsavedContent),
                Utils::clone(m_unsavedContent));
}

QString ClangQueryProjectsFindFilter::queryText() const
{
    return QString();
}

RefactoringClient &ClangQueryProjectsFindFilter::refactoringClient()
{
    return m_refactoringClient;
}


} // namespace ClangRefactoring
