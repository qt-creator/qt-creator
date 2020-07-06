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

#include "clangqueryhighlighter.h"
#include "clangqueryexamplehighlighter.h"

#include <clangrefactoringmessages.h>
#include <filepathcachinginterface.h>
#include <refactoringconnectionclient.h>

namespace ClangRefactoring {

void RefactoringClient::alive()
{
    if (m_connectionClient)
        m_connectionClient->resetProcessAliveTimer();
}

void RefactoringClient::sourceRangesAndDiagnosticsForQueryMessage(
        ClangBackEnd::SourceRangesAndDiagnosticsForQueryMessage &&message)
{
    m_clangQueryExampleHighlighter->setSourceRanges(message.takeSourceRanges());
    m_clangQueryHighlighter->setDiagnostics(message.diagnostics);
}

void RefactoringClient::sourceRangesForQueryMessage(ClangBackEnd::SourceRangesForQueryMessage &&message)
{
    ++m_resultCounter;
    addSearchResults(message.sourceRanges);
    setResultCounterAndSendSearchIsFinishedIfFinished();
}

void RefactoringClient::progress(ClangBackEnd::ProgressMessage &&message)
{
    m_progressManager.setProgress(message.progress, message.total);
}

void RefactoringClient::setRefactoringEngine(RefactoringEngine *refactoringEngine)
{
    m_refactoringEngine = refactoringEngine;
}

void RefactoringClient::setSearchHandle(SearchHandle *searchHandle)
{
    m_searchHandle = searchHandle;
}

SearchHandle *RefactoringClient::searchHandle() const
{
    return m_searchHandle;
}

void RefactoringClient::setClangQueryExampleHighlighter(ClangQueryExampleHighlighter *highlighter)
{
    m_clangQueryExampleHighlighter = highlighter;
}

void RefactoringClient::setClangQueryHighlighter(ClangQueryHighlighter *highlighter)
{
    m_clangQueryHighlighter = highlighter;
}

void RefactoringClient::setExpectedResultCount(uint count)
{
    m_expectedResultCount = count;
    m_resultCounter = 0;
    m_searchHandle->setExpectedResultCount(count);
}

uint RefactoringClient::expectedResultCount() const
{
    return m_expectedResultCount;
}

uint RefactoringClient::resultCounter() const
{
    return m_resultCounter;
}

void RefactoringClient::setRefactoringConnectionClient(
        ClangBackEnd::RefactoringConnectionClient *connectionClient)
{
    m_connectionClient = connectionClient;
}

void RefactoringClient::addSearchResults(const ClangBackEnd::SourceRangesContainer &sourceRanges)
{
    for (const auto &sourceRangeWithText : sourceRanges.sourceRangeWithTextContainers)
        addSearchResult(sourceRangeWithText);
}

void RefactoringClient::addSearchResult(const ClangBackEnd::SourceRangeWithTextContainer &sourceRangeWithText)
{
    auto &filePathCache = m_refactoringEngine->filePathCache();

    m_searchHandle->addResult(QString(filePathCache.filePath(sourceRangeWithText.filePathId()).path()),
                              QString(sourceRangeWithText.text),
                              {{int(sourceRangeWithText.start.line),
                                int(sourceRangeWithText.start.column - 1)},
                               {int(sourceRangeWithText.end.line),
                                int(sourceRangeWithText.end.column - 1)}});
}

void RefactoringClient::setResultCounterAndSendSearchIsFinishedIfFinished()
{
    m_searchHandle->setResultCounter(m_resultCounter);
    if (m_resultCounter == m_expectedResultCount)
        m_searchHandle->finishSearch();
}

} // namespace ClangRefactoring
