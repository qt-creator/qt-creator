/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "clangquerygatherer.h"

#include "clangquery.h"

namespace ClangBackEnd {

ClangQueryGatherer::ClangQueryGatherer(std::vector<V2::FileContainer> &&sources,
                                       std::vector<V2::FileContainer> &&unsaved,
                                       Utils::SmallString &&query)
    : m_sources(std::move(sources)),
      m_unsaved(std::move(unsaved)),
      m_query(std::move(query))
{
}

SourceRangesAndDiagnosticsForQueryMessage
ClangQueryGatherer::createSourceRangesAndDiagnosticsForSource(
        V2::FileContainer &&source,
        const std::vector<V2::FileContainer> &unsaved,
        Utils::SmallString &&query)
{
    ClangQuery clangQuery(std::move(query));

    clangQuery.addFile(source.filePath().directory(),
                       source.filePath().name(),
                       source.takeUnsavedFileContent(),
                       source.takeCommandLineArguments());

    clangQuery.addUnsavedFiles(unsaved);

    clangQuery.findLocations();

    return {clangQuery.takeSourceRanges(), clangQuery.takeDiagnosticContainers()};
}

bool ClangQueryGatherer::canCreateSourceRangesAndDiagnostics() const
{
    return !m_sources.empty();
}

SourceRangesAndDiagnosticsForQueryMessage ClangQueryGatherer::createNextSourceRangesAndDiagnostics()
{
    auto message = createSourceRangesAndDiagnosticsForSource(std::move(m_sources.back()),
                                                             m_unsaved,
                                                             m_query.clone());
    m_sources.pop_back();

    return message;
}

ClangQueryGatherer::Future ClangQueryGatherer::startCreateNextSourceRangesAndDiagnosticsMessage()
{
    Future future = std::async(std::launch::async,
                               createSourceRangesAndDiagnosticsForSource,
                               std::move(m_sources.back()),
                               m_unsaved,
                               m_query.clone());

    m_sources.pop_back();

    return future;
}

void ClangQueryGatherer::startCreateNextSourceRangesAndDiagnosticsMessages()
{
    std::vector<ClangQueryGatherer::Future> futures;

    while (!m_sources.empty() && m_sourceFutures.size() < m_processingSlotCount)
        m_sourceFutures.push_back(startCreateNextSourceRangesAndDiagnosticsMessage());
}

void ClangQueryGatherer::waitForFinished()
{
    for (Future &future : m_sourceFutures)
        future.wait();
}

bool ClangQueryGatherer::isFinished() const
{
    return m_sources.empty() && m_sourceFutures.empty();
}

const std::vector<V2::FileContainer> &ClangQueryGatherer::sources() const
{
    return m_sources;
}

const std::vector<ClangQueryGatherer::Future> &ClangQueryGatherer::sourceFutures() const
{
    return m_sourceFutures;
}

std::vector<SourceRangesAndDiagnosticsForQueryMessage> ClangQueryGatherer::allCurrentProcessedMessages()
{
    std::vector<SourceRangesAndDiagnosticsForQueryMessage> messages;

    for (Future &future : m_sourceFutures)
        messages.push_back(future.get());

    return messages;
}

std::vector<SourceRangesAndDiagnosticsForQueryMessage> ClangQueryGatherer::finishedMessages()
{
    std::vector<SourceRangesAndDiagnosticsForQueryMessage> messages;

    for (auto &&future : finishedFutures())
        messages.push_back(future.get());

    return messages;
}

void ClangQueryGatherer::setProcessingSlotCount(uint count)
{
    m_processingSlotCount = count;
}

std::vector<ClangQueryGatherer::Future> ClangQueryGatherer::finishedFutures()
{
    std::vector<Future> finishedFutures;
    finishedFutures.reserve(m_sourceFutures.size());

    auto beginReady = std::partition(m_sourceFutures.begin(),
                                     m_sourceFutures.end(),
                                     [] (const Future &future) {
        return future.wait_for(std::chrono::duration<int>::zero()) != std::future_status::ready;
    });

    std::move(beginReady, m_sourceFutures.end(), std::back_inserter(finishedFutures));
    m_sourceFutures.erase(beginReady, m_sourceFutures.end());

    return finishedFutures;
}

} // namespace ClangBackEnd
