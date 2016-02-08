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

#include "runextensions.h"

#include <QFutureWatcher>

namespace Utils {
namespace Internal {

// TODO: try to use this for replacing MultiTask

class MapReduceBase : public QObject
{
    Q_OBJECT
};

template <typename Container, typename MapFunction, typename State, typename ReduceResult, typename ReduceFunction>
class MapReduce : public MapReduceBase
{
    using MapResult = typename Internal::resultType<MapFunction>::type;
    using Iterator = typename Container::const_iterator;

public:
    MapReduce(QFutureInterface<ReduceResult> futureInterface, const Container &container,
              const MapFunction &map, State &state, const ReduceFunction &reduce)
        : m_futureInterface(futureInterface),
          m_container(container),
          m_iterator(m_container.begin()),
          m_map(map),
          m_state(state),
          m_reduce(reduce)
    {
        connect(&m_selfWatcher, &QFutureWatcher<void>::canceled,
                this, &MapReduce::cancelAll);
        m_selfWatcher.setFuture(futureInterface.future());
    }

    void exec()
    {
        if (schedule()) // do not enter event loop for empty containers
            m_loop.exec();
    }

private:
    bool schedule()
    {
        bool didSchedule = false;
        while (m_iterator != m_container.end() && m_mapWatcher.size() < QThread::idealThreadCount()) {
            didSchedule = true;
            auto watcher = new QFutureWatcher<MapResult>();
            connect(watcher, &QFutureWatcher<MapResult>::finished, this, [this, watcher]() {
                mapFinished(watcher);
            });
            m_mapWatcher.append(watcher);
            watcher->setFuture(runAsync(&m_threadPool, m_map, *m_iterator));
            ++m_iterator;
        }
        return didSchedule;
    }

    void mapFinished(QFutureWatcher<MapResult> *watcher)
    {
        m_mapWatcher.removeAll(watcher); // remove so we can schedule next one
        bool didSchedule = false;
        if (!m_futureInterface.isCanceled()) {
            // first schedule the next map...
            didSchedule = schedule();
            // ...then reduce
            const int resultCount = watcher->future().resultCount();
            for (int i = 0; i < resultCount; ++i) {
                Internal::runAsyncImpl(m_futureInterface, m_reduce, m_state, watcher->future().resultAt(i));
            }
        }
        delete watcher;
        if (!didSchedule && m_mapWatcher.isEmpty())
            m_loop.quit();
    }

    void cancelAll()
    {
        foreach (QFutureWatcher<MapResult> *watcher, m_mapWatcher)
            watcher->cancel();
    }

    QFutureWatcher<void> m_selfWatcher;
    QFutureInterface<ReduceResult> m_futureInterface;
    const Container &m_container;
    Iterator m_iterator;
    const MapFunction &m_map;
    State &m_state;
    const ReduceFunction &m_reduce;
    QEventLoop m_loop;
    QThreadPool m_threadPool; // for reusing threads
    QList<QFutureWatcher<MapResult> *> m_mapWatcher;
};

template <typename Container, typename InitFunction, typename MapFunction, typename ReduceResult,
          typename ReduceFunction, typename CleanUpFunction>
void blockingMapReduce(QFutureInterface<ReduceResult> &futureInterface, const Container &container,
                       const InitFunction &init, const MapFunction &map,
                       const ReduceFunction &reduce, const CleanUpFunction &cleanup)
{
    auto state = init(futureInterface);
    MapReduce<Container, MapFunction, decltype(state), ReduceResult, ReduceFunction> mr(futureInterface, container, map, state, reduce);
    mr.exec();
    cleanup(futureInterface, state);
}

} // Internal

template <typename Container, typename InitFunction, typename MapFunction,
          typename ReduceFunction, typename CleanUpFunction,
          typename ReduceResult = typename Internal::resultType<ReduceFunction>::type>
QFuture<ReduceResult>
mapReduce(std::reference_wrapper<Container> containerWrapper,
                                const InitFunction &init, const MapFunction &map,
                                const ReduceFunction &reduce, const CleanUpFunction &cleanup)
{
    return runAsync(Internal::blockingMapReduce<Container, InitFunction, MapFunction, ReduceResult, ReduceFunction, CleanUpFunction>,
                containerWrapper, init, map, reduce, cleanup);
}

/*!
    Calls the map function on all items in \a container in parallel through Utils::runAsync.

    The reduce function is called in the mapReduce thread with each of the reported results from
    the map function, in arbitrary order, but never in parallel.
    It gets passed a reference to a user defined state object, and a result from the map function.
    If it takes a QFutureInterface reference as its first argument, it can report results
    for the mapReduce operation through that. Otherwise, any values returned by the reduce function
    are reported as results of the mapReduce operation.

    The init function is called in the mapReduce thread before the actual mapping starts,
    and must return the initial state object for the reduce function. It gets the QFutureInterface
    of the mapReduce operation passed as an argument.

    The cleanup function is called in the mapReduce thread after all map and reduce calls have
    finished, with the QFutureInterface of the mapReduce operation and the final state object
    as arguments, and can be used to clean up any resources, or report a final result of the
    mapReduce.

    Container<ItemType>
    StateType InitFunction(QFutureInterface<ReduceResultType>&)

    void MapFunction(QFutureInterface<MapResultType>&, const ItemType&)
    or
    MapResultType MapFunction(const ItempType&)

    void ReduceFunction(QFutureInterface<ReduceResultType>&, StateType&, const ItemType&)
    or
    ReduceResultType ReduceFunction(StateType&, const ItemType&)

    void CleanUpFunction(QFutureInterface<ReduceResultType>&, StateType&)
 */
template <typename Container, typename InitFunction, typename MapFunction,
          typename ReduceFunction, typename CleanUpFunction,
          typename ReduceResult = typename Internal::resultType<ReduceFunction>::type>
QFuture<ReduceResult>
mapReduce(const Container &container, const InitFunction &init, const MapFunction &map,
               const ReduceFunction &reduce, const CleanUpFunction &cleanup)
{
    return runAsync(Internal::blockingMapReduce<Container, InitFunction, MapFunction, ReduceResult, ReduceFunction, CleanUpFunction>,
                container, init, map, reduce, cleanup);
}

} // Utils
