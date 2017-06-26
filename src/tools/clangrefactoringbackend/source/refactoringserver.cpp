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

#include <functional>

namespace ClangBackEnd {

RefactoringServer::RefactoringServer()
{
    m_pollTimer.setInterval(100);

    QObject::connect(&m_pollTimer,
                     &QTimer::timeout,
                     std::bind(&RefactoringServer::pollSourceRangesAndDiagnosticsForQueryMessages, this));
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
    gatherSourceRangesAndDiagnosticsForQueryMessages(message.takeSources(),
                                                     message.takeUnsavedContent(),
                                                     message.takeQuery());
}

void RefactoringServer::cancel()
{
    m_gatherer.waitForFinished();
    m_gatherer = ClangQueryGatherer();
    m_pollTimer.stop();
}

bool RefactoringServer::isCancelingJobs() const
{
    return m_gatherer.isFinished();
}

void RefactoringServer::pollSourceRangesAndDiagnosticsForQueryMessages()
{
    for (auto &&message : m_gatherer.finishedMessages())
        client()->sourceRangesAndDiagnosticsForQueryMessage(std::move(message));

    if (!m_gatherer.isFinished())
        m_gatherer.startCreateNextSourceRangesAndDiagnosticsMessages();
    else
        m_pollTimer.stop();
}

void RefactoringServer::waitThatSourceRangesAndDiagnosticsForQueryMessagesAreFinished()
{
    while (!m_gatherer.isFinished()) {
        m_gatherer.waitForFinished();
        pollSourceRangesAndDiagnosticsForQueryMessages();
    }
}

bool RefactoringServer::pollTimerIsActive() const
{
    return m_pollTimer.isActive();
}

void RefactoringServer::gatherSourceRangesAndDiagnosticsForQueryMessages(
        std::vector<V2::FileContainer> &&sources,
        std::vector<V2::FileContainer> &&unsaved,
        Utils::SmallString &&query)
{
#ifdef _WIN32
    uint freeProcessors = 1;
#else
    uint freeProcessors = std::thread::hardware_concurrency();
#endif

    m_gatherer = ClangQueryGatherer(std::move(sources), std::move(unsaved), std::move(query));
    m_gatherer.setProcessingSlotCount(freeProcessors);

    m_pollTimer.start();
}

} // namespace ClangBackEnd
