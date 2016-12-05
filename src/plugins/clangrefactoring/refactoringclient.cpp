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

#include "refactoringclient.h"

#include <refactoringconnectionclient.h>
#include <sourcelocationsforrenamingmessage.h>
#include <sourcerangesanddiagnosticsforquerymessage.h>

namespace ClangRefactoring {

void RefactoringClient::alive()
{
    if (connectionClient)
        connectionClient->resetProcessAliveTimer();
}

void RefactoringClient::sourceLocationsForRenamingMessage(
        ClangBackEnd::SourceLocationsForRenamingMessage &&message)
{
    localRenamingCallback(message.symbolName().toQString(),
                          message.sourceLocations(),
                          message.textDocumentRevision());

    refactoringEngine->setUsable(true);
}

void RefactoringClient::sourceRangesAndDiagnosticsForQueryMessage(
        ClangBackEnd::SourceRangesAndDiagnosticsForQueryMessage &&message)
{
    ++resultCounter_;
    addSearchResults(message.sourceRanges());
    setResultCounterAndSendSearchIsFinishedIfFinished();
}

void RefactoringClient::setLocalRenamingCallback(
        CppTools::RefactoringEngineInterface::RenameCallback &&localRenamingCallback)
{
    this->localRenamingCallback = std::move(localRenamingCallback);
}

void RefactoringClient::setRefactoringEngine(RefactoringEngine *refactoringEngine)
{
    this->refactoringEngine = refactoringEngine;
}

void RefactoringClient::setSearchHandle(SearchHandle *searchHandle)
{
    this->searchHandle_ = searchHandle;
}

SearchHandle *RefactoringClient::searchHandle() const
{
    return searchHandle_;
}

bool RefactoringClient::hasValidLocalRenamingCallback() const
{
    return bool(localRenamingCallback);
}

void RefactoringClient::setExpectedResultCount(uint count)
{
    expectedResultCount_ = count;
    resultCounter_ = 0;
    searchHandle_->setExpectedResultCount(count);
}

uint RefactoringClient::expectedResultCount() const
{
    return expectedResultCount_;
}

uint RefactoringClient::resultCounter() const
{
    return resultCounter_;
}

void RefactoringClient::setRefactoringConnectionClient(
        ClangBackEnd::RefactoringConnectionClient *connectionClient)
{
    this->connectionClient = connectionClient;
}

namespace {

Utils::SmallString concatenateFilePath(const ClangBackEnd::FilePath &filePath)
{
    Utils::SmallString concatenatedFilePath = filePath.directory().clone();
    concatenatedFilePath.append("/");
    concatenatedFilePath.append(filePath.name().clone());

    return concatenatedFilePath;
}

}

std::unordered_map<uint, QString> RefactoringClient::convertFilePaths(
        const ClangBackEnd::FilePathDict &filePaths)
{
    using Dict = std::unordered_map<uint, QString>;
    Dict qstringFilePaths;
    qstringFilePaths.reserve(filePaths.size());

    auto convertFilePath = [] (const ClangBackEnd::FilePathDict::value_type &dictonaryEntry) {
        return std::make_pair(dictonaryEntry.first,
                              concatenateFilePath(dictonaryEntry.second).toQString());
    };

    std::transform(filePaths.begin(),
                   filePaths.end(),
                   std::inserter(qstringFilePaths, qstringFilePaths.begin()),
                   convertFilePath);

    return qstringFilePaths;
}

void RefactoringClient::addSearchResults(const ClangBackEnd::SourceRangesContainer &sourceRanges)
{
    auto filePaths = convertFilePaths(sourceRanges.filePaths());

    for (const auto &sourceRangeWithText : sourceRanges.sourceRangeWithTextContainers())
        addSearchResult(sourceRangeWithText, filePaths);
}

void RefactoringClient::addSearchResult(const ClangBackEnd::SourceRangeWithTextContainer &sourceRangeWithText,
                                        std::unordered_map<uint, QString> &filePaths)
{
    searchHandle_->addResult(filePaths[sourceRangeWithText.fileHash()],
                             sourceRangeWithText.text(),
                             {{int(sourceRangeWithText.start().line()),
                               int(sourceRangeWithText.start().column() - 1),
                               int(sourceRangeWithText.start().offset())},
                              {int(sourceRangeWithText.end().line()),
                               int(sourceRangeWithText.end().column() - 1),
                               int(sourceRangeWithText.end().offset())}});
}

void RefactoringClient::setResultCounterAndSendSearchIsFinishedIfFinished()
{
    searchHandle_->setResultCounter(resultCounter_);
    if (resultCounter_ == expectedResultCount_)
        searchHandle_->finishSearch();
}

} // namespace ClangRefactoring
