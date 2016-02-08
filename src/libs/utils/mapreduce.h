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

#pragma once

#include "qtcassert.h"

#include <QFuture>
#include <QFutureInterface>

#include <chrono>
#include <future>
#include <thread>
#include <vector>

namespace Utils {

template<typename T>
typename std::vector<std::future<T>>::iterator
waitForAny(std::vector<std::future<T>> &futures)
{
    // Wait for any future to have a result ready.
    // Unfortunately we have to do that in a busy loop because future doesn't have a feature to
    // wait for any of a set of futures (yet? possibly when_any in C++17).
    auto end = futures.end();
    QTC_ASSERT(!futures.empty(), return end);
    auto futureIterator = futures.begin();
    forever {
        if (futureIterator->wait_for(std::chrono::duration<quint64>::zero()) == std::future_status::ready)
            return futureIterator;
        ++futureIterator;
        if (futureIterator == end)
            futureIterator = futures.begin();
    }
}

namespace Internal {

template<typename T>
void swapErase(std::vector<T> &vec, typename std::vector<T>::iterator it)
{
    // efficient erasing by swapping with back element
    *it = std::move(vec.back());
    vec.pop_back();
}

template <typename MapResult, typename State, typename ReduceResult, typename ReduceFunction>
void reduceOne(QFutureInterface<ReduceResult> &futureInterface,
               std::vector<std::future<MapResult>> &futures,
               State &state, const ReduceFunction &reduce)
{
    auto futureIterator = waitForAny(futures);
    if (futureIterator != futures.end()) {
        reduce(futureInterface, state, futureIterator->get());
        swapErase(futures, futureIterator);
    }
}

// This together with reduceOne can be replaced by std::transformReduce (parallelism TS)
// when that becomes widely available in C++ implementations
template <typename Container, typename MapFunction, typename State, typename ReduceResult, typename ReduceFunction>
void mapReduceLoop(QFutureInterface<ReduceResult> &futureInterface, const Container &container,
               const MapFunction &map, State &state, const ReduceFunction &reduce)
{
    const unsigned MAX_THREADS = std::thread::hardware_concurrency();
    using MapResult = typename std::result_of<MapFunction(QFutureInterface<ReduceResult>,typename Container::value_type)>::type;
    std::vector<std::future<MapResult>> futures;
    futures.reserve(MAX_THREADS);
    auto fileIterator = container.begin();
    auto end = container.end();
    while (!futureInterface.isCanceled() && (fileIterator != end || futures.size() != 0)) {
        if (futures.size() >= MAX_THREADS || fileIterator == end) {
            // We don't want to start a new thread (yet), so try to find a future that is ready and
            // handle its result.
            reduceOne(futureInterface, futures, state, reduce);
        } else { // start a new thread
            futures.push_back(std::async(std::launch::async,
                                         map, futureInterface, *fileIterator));
            ++fileIterator;
        }
    }
}

template <typename Container, typename InitFunction, typename MapFunction, typename ReduceResult,
          typename ReduceFunction, typename CleanUpFunction>
void blockingMapReduce(QFutureInterface<ReduceResult> futureInterface, const Container &container,
                       const InitFunction &init, const MapFunction &map,
                       const ReduceFunction &reduce, const CleanUpFunction &cleanup)
{
    auto state = init(futureInterface);
    mapReduceLoop(futureInterface, container, map, state, reduce);
    cleanup(futureInterface, state);
    if (futureInterface.isPaused())
        futureInterface.waitForResume();
    futureInterface.reportFinished();
}

} // Internal

template <typename ReduceResult, typename Container, typename InitFunction, typename MapFunction,
          typename ReduceFunction, typename CleanUpFunction>
QFuture<ReduceResult> mapReduce(std::reference_wrapper<Container> containerWrapper,
                                const InitFunction &init, const MapFunction &map,
                                const ReduceFunction &reduce, const CleanUpFunction &cleanup)
{
    auto fi = QFutureInterface<ReduceResult>();
    QFuture<ReduceResult> future = fi.future();
    fi.reportStarted();
    std::thread(Internal::blockingMapReduce<Container, InitFunction, MapFunction, ReduceResult, ReduceFunction, CleanUpFunction>,
                fi, containerWrapper, init, map, reduce, cleanup).detach();
    return future;
}

template <typename ReduceResult, typename Container, typename InitFunction, typename MapFunction,
          typename ReduceFunction, typename CleanUpFunction>
QFuture<ReduceResult> mapReduce(const Container &container, const InitFunction &init, const MapFunction &map,
               const ReduceFunction &reduce, const CleanUpFunction &cleanup)
{
    auto fi = QFutureInterface<ReduceResult>();
    QFuture<ReduceResult> future = fi.future();
    std::thread(Internal::blockingMapReduce<Container, InitFunction, MapFunction, ReduceResult, ReduceFunction, CleanUpFunction>,
                fi, container, init, map, reduce, cleanup).detach();
    return future;
}

} // Utils
