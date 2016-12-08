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

#include "refactoringserver.h"

#include "symbolfinder.h"
#include "clangquery.h"

#include <refactoringclientinterface.h>
#include <requestsourcelocationforrenamingmessage.h>
#include <requestsourcerangesanddiagnosticsforquerymessage.h>
#include <sourcelocationsforrenamingmessage.h>
#include <sourcerangesanddiagnosticsforquerymessage.h>

#include <QCoreApplication>

#include <algorithm>
#include <chrono>
#include <future>
#include <atomic>

namespace ClangBackEnd {

RefactoringServer::RefactoringServer()
{
    pollEventLoop = [] () { QCoreApplication::processEvents(); };
}

void RefactoringServer::end()
{
    QCoreApplication::exit();
}

void RefactoringServer::requestSourceLocationsForRenamingMessage(RequestSourceLocationsForRenamingMessage &&message)
{
    SymbolFinder symbolFinder(message.line(), message.column());

    symbolFinder.addFile(message.filePath().directory(),
                         message.filePath().name(),
                         message.unsavedContent(),
                         message.commandLine());

    symbolFinder.findSymbol();

    client()->sourceLocationsForRenamingMessage({symbolFinder.takeSymbolName(),
                                                 symbolFinder.takeSourceLocations(),
                                                 message.textDocumentRevision()});
}

void RefactoringServer::requestSourceRangesAndDiagnosticsForQueryMessage(
        RequestSourceRangesAndDiagnosticsForQueryMessage &&message)
{
    gatherSourceRangesAndDiagnosticsForQueryMessage(message.takeSources(),
                                                    message.takeUnsavedContent(),
                                                    message.takeQuery());
}

void RefactoringServer::cancel()
{
    cancelWork = true;
}

bool RefactoringServer::isCancelingJobs() const
{
    return cancelWork;
}

void RefactoringServer::supersedePollEventLoop(std::function<void ()> &&pollEventLoop)
{
    this->pollEventLoop = std::move(pollEventLoop);
}

namespace {

SourceRangesAndDiagnosticsForQueryMessage createSourceRangesAndDiagnosticsForQueryMessage(
        V2::FileContainer &&source,
        std::vector<V2::FileContainer> &&unsaved,
        Utils::SmallString &&query,
        const std::atomic_bool &cancelWork) {
    ClangQuery clangQuery(std::move(query));

    if (!cancelWork) {
        clangQuery.addFile(source.filePath().directory(),
                           source.filePath().name(),
                           source.takeUnsavedFileContent(),
                           source.takeCommandLineArguments());

        clangQuery.addUnsavedFiles(std::move(unsaved));

        clangQuery.findLocations();
    }

    return {clangQuery.takeSourceRanges(), clangQuery.takeDiagnosticContainers()};
}


}

void RefactoringServer::gatherSourceRangesAndDiagnosticsForQueryMessage(
        std::vector<V2::FileContainer> &&sources,
        std::vector<V2::FileContainer> &&unsaved,
        Utils::SmallString &&query)
{
    std::vector<Future> futures;

#ifdef _WIN32
    std::size_t freeProcessors = 1;
#else
    std::size_t freeProcessors = std::thread::hardware_concurrency();
#endif

    while (!sources.empty() || !futures.empty()) {
        --freeProcessors;

        if (!sources.empty()) {
            Future &&future = std::async(std::launch::async,
                                         createSourceRangesAndDiagnosticsForQueryMessage,
                                         std::move(sources.back()),
                                         Utils::clone(unsaved),
                                         query.clone(),
                                         std::ref(cancelWork));
            sources.pop_back();

            futures.emplace_back(std::move(future));
        }

        if (freeProcessors == 0 || sources.empty())
            freeProcessors += waitForNewSourceRangesAndDiagnosticsForQueryMessage(futures);
    }
}

std::size_t RefactoringServer::waitForNewSourceRangesAndDiagnosticsForQueryMessage(std::vector<Future> &futures)
{
    while (true) {
        pollEventLoop();

        std::vector<Future> readyFutures;
        readyFutures.reserve(futures.size());

        auto beginReady = std::partition(futures.begin(),
                                         futures.end(),
                                         [] (const Future &future) {
            return future.wait_for(std::chrono::duration<int>::zero()) != std::future_status::ready;
        });

        std::move(beginReady, futures.end(), std::back_inserter(readyFutures));
        futures.erase(beginReady, futures.end());

        for (Future &readyFuture : readyFutures)
            client()->sourceRangesAndDiagnosticsForQueryMessage(readyFuture.get());

        if (readyFutures.empty())
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        else
            return readyFutures.size();
    }
}

} // namespace ClangBackEnd
