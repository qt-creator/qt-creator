/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "symbolindexertaskscheduler.h"

#include <symbolscollectormanagerinterface.h>
#include <symbolscollectorinterface.h>

#include <algorithm>
#include <thread>

namespace ClangBackEnd {

void SymbolIndexerTaskScheduler::addTasks(std::vector<Task> &&tasks)
{
    for (auto &task : tasks) {
        auto callWrapper = [task=std::move(task)] (
                std::reference_wrapper<SymbolsCollectorInterface> symbolsCollector,
                std::reference_wrapper<SymbolStorageInterface> symbolStorage)
                -> SymbolsCollectorInterface& {
            task(symbolsCollector.get(), symbolStorage.get());
            return symbolsCollector;
        };
        m_futures.emplace_back(std::async(m_launchPolicy,
                                          std::move(callWrapper),
                                          std::ref(m_symbolsCollectorManager.unusedSymbolsCollector()),
                                          std::ref(m_symbolStorage)));
    }
}

const std::vector<SymbolIndexerTaskScheduler::Future> &SymbolIndexerTaskScheduler::futures() const
{
    return m_futures;
}

int SymbolIndexerTaskScheduler::freeSlots()
{
    removeFinishedFutures();

    return std::max(m_hardware_concurrency - int(m_futures.size()), 0);
}

void SymbolIndexerTaskScheduler::syncTasks()
{
    for (auto &future : m_futures)
        future.wait();
}

void SymbolIndexerTaskScheduler::removeFinishedFutures()
{
    auto notReady = [] (Future &future) {
        return future.wait_for(std::chrono::duration<int>::zero()) != std::future_status::ready;
    };

    auto split = std::partition(m_futures.begin(), m_futures.end(), notReady);

    std::for_each(split, m_futures.end(), [] (Future &future) {
        future.get().setIsUsed(false);
    });

    m_futures.erase(split, m_futures.end());
}

} // namespace ClangBackEnd
