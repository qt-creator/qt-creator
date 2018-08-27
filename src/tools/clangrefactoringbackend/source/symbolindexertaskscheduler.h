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

#pragma once

#include <functional>
#include <future>
#include <vector>

namespace ClangBackEnd {

class FilePathCachingInterface;
class SymbolsCollectorInterface;
class SymbolsCollectorManagerInterface;
class SymbolIndexerTaskQueueInterface;
class SymbolStorageInterface;

class SymbolIndexerTaskScheduler
{
public:
    using Task = std::function<void(SymbolsCollectorInterface &symbolsCollector,
                                    SymbolStorageInterface &symbolStorage)>;
    using Future = std::future<SymbolsCollectorInterface&>;

    SymbolIndexerTaskScheduler(SymbolsCollectorManagerInterface &symbolsCollectorManager,
                               SymbolStorageInterface &symbolStorage,
                               SymbolIndexerTaskQueueInterface &symbolIndexerTaskQueue,
                               int hardware_concurrency,
                               std::launch launchPolicy = std::launch::async)
        : m_symbolsCollectorManager(symbolsCollectorManager),
          m_symbolStorage(symbolStorage),
          m_symbolIndexerTaskQueue(symbolIndexerTaskQueue),
          m_hardware_concurrency(hardware_concurrency),
          m_launchPolicy(launchPolicy)
    {}

    void addTasks(std::vector<Task> &&tasks);

    const std::vector<Future> &futures() const;

    int freeSlots();

    void syncTasks();

private:
    void removeFinishedFutures();

private:
    std::vector<Future> m_futures;
    SymbolsCollectorManagerInterface &m_symbolsCollectorManager;
    SymbolStorageInterface &m_symbolStorage;
    SymbolIndexerTaskQueueInterface &m_symbolIndexerTaskQueue;
    int m_hardware_concurrency;
    std::launch m_launchPolicy;
};

} // namespace ClangBackEnd
