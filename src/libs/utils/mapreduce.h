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

#include "utils_global.h"
#include "algorithm.h"
#include "runextensions.h"

#include <QFutureWatcher>

namespace Utils {

enum class MapReduceOption
{
    Ordered,
    Unordered
};

namespace Internal {

class QTCREATOR_UTILS_EXPORT MapReduceObject : public QObject
{
    Q_OBJECT
};

template <typename ForwardIterator, typename MapResult, typename MapFunction, typename State, typename ReduceResult, typename ReduceFunction>
class MapReduceBase : public MapReduceObject
{
protected:
    static const int MAX_PROGRESS = 1000000;
    // either const or non-const reference wrapper for items from the iterator
    using ItemReferenceWrapper = std::reference_wrapper<std::remove_reference_t<typename ForwardIterator::reference>>;

public:
    MapReduceBase(QFutureInterface<ReduceResult> futureInterface, ForwardIterator begin, ForwardIterator end,
                  MapFunction &&map, State &state, ReduceFunction &&reduce,
                  MapReduceOption option, QThreadPool *pool, int size)
        : m_futureInterface(futureInterface),
          m_iterator(begin),
          m_end(end),
          m_map(std::forward<MapFunction>(map)),
          m_state(state),
          m_reduce(std::forward<ReduceFunction>(reduce)),
          m_threadPool(pool),
          m_handleProgress(size >= 0),
          m_size(size),
          m_option(option)
    {
        if (!m_threadPool)
            m_threadPool = new QThreadPool(this);
        if (m_handleProgress) // progress is handled by us
            m_futureInterface.setProgressRange(0, MAX_PROGRESS);
        connect(&m_selfWatcher, &QFutureWatcher<void>::canceled,
                this, &MapReduceBase::cancelAll);
        m_selfWatcher.setFuture(futureInterface.future());
    }

    void exec()
    {
        // do not enter event loop for empty containers or if already canceled
        if (!m_futureInterface.isCanceled() && schedule())
            m_loop.exec();
    }

protected:
    virtual void reduce(QFutureWatcher<MapResult> *watcher, int index) = 0;

    bool schedule()
    {
        bool didSchedule = false;
        while (m_iterator != m_end
               && m_mapWatcher.size() < std::max(m_threadPool->maxThreadCount(), 1)) {
            didSchedule = true;
            auto watcher = new QFutureWatcher<MapResult>();
            connect(watcher, &QFutureWatcher<MapResult>::finished, this, [this, watcher]() {
                mapFinished(watcher);
            });
            if (m_handleProgress) {
                connect(watcher, &QFutureWatcher<MapResult>::progressValueChanged,
                        this, &MapReduceBase::updateProgress);
                connect(watcher, &QFutureWatcher<MapResult>::progressRangeChanged,
                        this, &MapReduceBase::updateProgress);
            }
            m_mapWatcher.append(watcher);
            m_watcherIndex.append(m_currentIndex);
            ++m_currentIndex;
            watcher->setFuture(runAsync(m_threadPool, std::cref(m_map),
                                        ItemReferenceWrapper(*m_iterator)));
            ++m_iterator;
        }
        return didSchedule;
    }

    void mapFinished(QFutureWatcher<MapResult> *watcher)
    {
        int index = m_mapWatcher.indexOf(watcher);
        int watcherIndex = m_watcherIndex.at(index);
        m_mapWatcher.removeAt(index); // remove so we can schedule next one
        m_watcherIndex.removeAt(index);
        bool didSchedule = false;
        if (!m_futureInterface.isCanceled()) {
            // first schedule the next map...
            didSchedule = schedule();
            ++m_successfullyFinishedMapCount;
            updateProgress();
            // ...then reduce
            reduce(watcher, watcherIndex);
        }
        delete watcher;
        if (!didSchedule && m_mapWatcher.isEmpty())
            m_loop.quit();
    }

    void updateProgress()
    {
        if (!m_handleProgress) // cannot compute progress
            return;
        if (m_size == 0 || m_successfullyFinishedMapCount == m_size) {
            m_futureInterface.setProgressValue(MAX_PROGRESS);
            return;
        }
        if (!m_futureInterface.isProgressUpdateNeeded())
            return;
        const double progressPerMap = MAX_PROGRESS / double(m_size);
        double progress = m_successfullyFinishedMapCount * progressPerMap;
        foreach (const QFutureWatcher<MapResult> *watcher, m_mapWatcher) {
            if (watcher->progressMinimum() != watcher->progressMaximum()) {
                const double range = watcher->progressMaximum() - watcher->progressMinimum();
                progress += (watcher->progressValue() - watcher->progressMinimum()) / range * progressPerMap;
            }
        }
        m_futureInterface.setProgressValue(int(progress));
    }

    void cancelAll()
    {
        foreach (QFutureWatcher<MapResult> *watcher, m_mapWatcher)
            watcher->cancel();
    }

    QFutureWatcher<void> m_selfWatcher;
    QFutureInterface<ReduceResult> m_futureInterface;
    ForwardIterator m_iterator;
    const ForwardIterator m_end;
    MapFunction m_map;
    State &m_state;
    ReduceFunction m_reduce;
    QEventLoop m_loop;
    QThreadPool *m_threadPool; // for reusing threads
    QList<QFutureWatcher<MapResult> *> m_mapWatcher;
    QList<int> m_watcherIndex;
    int m_currentIndex = 0;
    const bool m_handleProgress;
    const int m_size;
    int m_successfullyFinishedMapCount = 0;
    MapReduceOption m_option;
};

// non-void result of map function.
template <typename ForwardIterator, typename MapResult, typename MapFunction, typename State, typename ReduceResult, typename ReduceFunction>
class MapReduce : public MapReduceBase<ForwardIterator, MapResult, MapFunction, State, ReduceResult, ReduceFunction>
{
    using BaseType = MapReduceBase<ForwardIterator, MapResult, MapFunction, State, ReduceResult, ReduceFunction>;
public:
    MapReduce(QFutureInterface<ReduceResult> futureInterface, ForwardIterator begin, ForwardIterator end,
              MapFunction &&map, State &state, ReduceFunction &&reduce, MapReduceOption option,
              QThreadPool *pool, int size)
        : BaseType(futureInterface, begin, end, std::forward<MapFunction>(map), state,
                   std::forward<ReduceFunction>(reduce), option, pool, size)
    {
    }

protected:
    void reduce(QFutureWatcher<MapResult> *watcher, int index) override
    {
        if (BaseType::m_option == MapReduceOption::Unordered) {
            reduceOne(watcher->future().results());
        } else {
            if (m_nextIndex == index) {
                // handle this result and all directly following
                reduceOne(watcher->future().results());
                ++m_nextIndex;
                while (!m_pendingResults.isEmpty() && m_pendingResults.firstKey() == m_nextIndex) {
                    reduceOne(m_pendingResults.take(m_nextIndex));
                    ++m_nextIndex;
                }
            } else {
                // add result to pending results
                m_pendingResults.insert(index, watcher->future().results());
            }
        }
    }

private:
    void reduceOne(const QList<MapResult> &results)
    {
        const int resultCount = results.size();
        for (int i = 0; i < resultCount; ++i) {
            Internal::runAsyncImpl(BaseType::m_futureInterface, BaseType::m_reduce,
                                   BaseType::m_state, results.at(i));
        }
    }

    QMap<int, QList<MapResult>> m_pendingResults;
    int m_nextIndex = 0;
};

// specialization for void result of map function. Reducing is a no-op.
template <typename ForwardIterator, typename MapFunction, typename State, typename ReduceResult, typename ReduceFunction>
class MapReduce<ForwardIterator, void, MapFunction, State, ReduceResult, ReduceFunction> : public MapReduceBase<ForwardIterator, void, MapFunction, State, ReduceResult, ReduceFunction>
{
    using BaseType = MapReduceBase<ForwardIterator, void, MapFunction, State, ReduceResult, ReduceFunction>;
public:
    MapReduce(QFutureInterface<ReduceResult> futureInterface, ForwardIterator begin, ForwardIterator end,
              MapFunction &&map, State &state, ReduceFunction &&reduce, MapReduceOption option,
              QThreadPool *pool, int size)
        : BaseType(futureInterface, begin, end, std::forward<MapFunction>(map), state,
                   std::forward<ReduceFunction>(reduce), option, pool, size)
    {
    }

protected:
    void reduce(QFutureWatcher<void> *, int) override
    {
    }

};

template <typename ResultType, typename Function, typename... Args>
functionResult_t<Function>
callWithMaybeFutureInterfaceDispatch(std::false_type, QFutureInterface<ResultType> &,
                                     Function &&function, Args&&... args)
{
    return function(std::forward<Args>(args)...);
}

template <typename ResultType, typename Function, typename... Args>
functionResult_t<Function>
callWithMaybeFutureInterfaceDispatch(std::true_type, QFutureInterface<ResultType> &futureInterface,
                                     Function &&function, Args&&... args)
{
    return function(futureInterface, std::forward<Args>(args)...);
}

template <typename ResultType, typename Function, typename... Args>
functionResult_t<Function>
callWithMaybeFutureInterface(QFutureInterface<ResultType> &futureInterface,
                             Function &&function, Args&&... args)
{
    return callWithMaybeFutureInterfaceDispatch(
                functionTakesArgument<Function, 0, QFutureInterface<ResultType>&>(),
                futureInterface, std::forward<Function>(function), std::forward<Args>(args)...);
}

template <typename ForwardIterator, typename InitFunction, typename MapFunction, typename ReduceResult,
          typename ReduceFunction, typename CleanUpFunction>
void blockingIteratorMapReduce(QFutureInterface<ReduceResult> &futureInterface, ForwardIterator begin, ForwardIterator end,
                               InitFunction &&init, MapFunction &&map,
                               ReduceFunction &&reduce, CleanUpFunction &&cleanup,
                               MapReduceOption option, QThreadPool *pool, int size)
{
    auto state = callWithMaybeFutureInterface<ReduceResult, InitFunction>
            (futureInterface, std::forward<InitFunction>(init));
    MapReduce<ForwardIterator, typename Internal::resultType<MapFunction>::type, MapFunction, decltype(state), ReduceResult, ReduceFunction>
            mr(futureInterface, begin, end, std::forward<MapFunction>(map), state,
               std::forward<ReduceFunction>(reduce), option, pool, size);
    mr.exec();
    callWithMaybeFutureInterface<ReduceResult, CleanUpFunction, std::remove_reference_t<decltype(state)>&>
            (futureInterface, std::forward<CleanUpFunction>(cleanup), state);
}

template <typename Container, typename InitFunction, typename MapFunction, typename ReduceResult,
          typename ReduceFunction, typename CleanUpFunction>
void blockingContainerMapReduce(QFutureInterface<ReduceResult> &futureInterface, Container &&container,
                                InitFunction &&init, MapFunction &&map,
                                ReduceFunction &&reduce, CleanUpFunction &&cleanup,
                                MapReduceOption option, QThreadPool *pool)
{
    blockingIteratorMapReduce(futureInterface, std::begin(container), std::end(container),
                              std::forward<InitFunction>(init), std::forward<MapFunction>(map),
                              std::forward<ReduceFunction>(reduce),
                              std::forward<CleanUpFunction>(cleanup),
                              option, pool, static_cast<int>(container.size()));
}

template <typename Container, typename InitFunction, typename MapFunction, typename ReduceResult,
          typename ReduceFunction, typename CleanUpFunction>
void blockingContainerRefMapReduce(QFutureInterface<ReduceResult> &futureInterface,
                                    std::reference_wrapper<Container> containerWrapper,
                                    InitFunction &&init, MapFunction &&map,
                                    ReduceFunction &&reduce, CleanUpFunction &&cleanup,
                                    MapReduceOption option, QThreadPool *pool)
{
    blockingContainerMapReduce(futureInterface, containerWrapper.get(),
                               std::forward<InitFunction>(init), std::forward<MapFunction>(map),
                               std::forward<ReduceFunction>(reduce),
                               std::forward<CleanUpFunction>(cleanup),
                               option, pool);
}

template <typename ReduceResult>
static void *dummyInit() { return nullptr; }

// copies or moves state to member, and then moves it to the result of the call operator
template <typename State>
struct StateWrapper {
    using StateResult = std::decay_t<State>; // State is const& or & for lvalues
    StateWrapper(State &&state) : m_state(std::forward<State>(state)) { }
    StateResult operator()()
    {
        return std::move(m_state); // invalidates m_state
    }

    StateResult m_state;
};

// copies or moves reduce function to member, calls the reduce function with state and mapped value
template <typename StateResult, typename MapResult, typename ReduceFunction>
struct ReduceWrapper {
    using Reduce = std::decay_t<ReduceFunction>;
    ReduceWrapper(ReduceFunction &&reduce) : m_reduce(std::forward<ReduceFunction>(reduce)) { }
    void operator()(QFutureInterface<StateResult> &, StateResult &state, const MapResult &mapResult)
    {
        m_reduce(state, mapResult);
    }

    Reduce m_reduce;
};

template <typename MapResult>
struct DummyReduce {
    MapResult operator()(void *, const MapResult &result) const { return result; }
};
template <>
struct DummyReduce<void> {
    void operator()() const { } // needed for resultType<DummyReduce> with MSVC2013
};

template <typename ReduceResult>
static void dummyCleanup(void *) { }

template <typename StateResult>
static void cleanupReportingState(QFutureInterface<StateResult> &fi, StateResult &state)
{
    fi.reportResult(state);
}

} // Internal

template <typename ForwardIterator, typename InitFunction, typename MapFunction,
          typename ReduceFunction, typename CleanUpFunction,
          typename ReduceResult = typename Internal::resultType<ReduceFunction>::type>
QFuture<ReduceResult>
mapReduce(ForwardIterator begin, ForwardIterator end, InitFunction &&init, MapFunction &&map,
          ReduceFunction &&reduce, CleanUpFunction &&cleanup,
          MapReduceOption option = MapReduceOption::Unordered,
          QThreadPool *pool = nullptr, QThread::Priority priority = QThread::InheritPriority,
          int size = -1)
{
    return runAsync(priority,
                    Internal::blockingIteratorMapReduce<
                        ForwardIterator,
                        std::decay_t<InitFunction>,
                        std::decay_t<MapFunction>,
                        std::decay_t<ReduceResult>,
                        std::decay_t<ReduceFunction>,
                        std::decay_t<CleanUpFunction>>,
                    begin, end, std::forward<InitFunction>(init), std::forward<MapFunction>(map),
                    std::forward<ReduceFunction>(reduce), std::forward<CleanUpFunction>(cleanup),
                    option, pool, size);
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
    or
    StateType InitFunction()

    void MapFunction(QFutureInterface<MapResultType>&, const ItemType&)
    or
    MapResultType MapFunction(const ItempType&)

    void ReduceFunction(QFutureInterface<ReduceResultType>&, StateType&, const MapResultType&)
    or
    ReduceResultType ReduceFunction(StateType&, const MapResultType&)

    void CleanUpFunction(QFutureInterface<ReduceResultType>&, StateType&)
    or
    void CleanUpFunction(StateType&)

    Notes:
    \list
        \li Container can be a move-only type or a temporary. If it is a lvalue reference, it will
            be copied to the mapReduce thread. You can avoid that by using
            the version that takes iterators, or by using std::ref/cref to pass a reference_wrapper.
        \li ItemType can be a move-only type, if the map function takes (const) references to ItemType.
        \li StateType can be a move-only type.
        \li The init, map, reduce and cleanup functions can be move-only types and are moved to the
            mapReduce thread if they are rvalues.
    \endlist

 */
template <typename Container, typename InitFunction, typename MapFunction,
          typename ReduceFunction, typename CleanUpFunction,
          typename ReduceResult = typename Internal::resultType<ReduceFunction>::type>
QFuture<ReduceResult>
mapReduce(Container &&container, InitFunction &&init, MapFunction &&map,
          ReduceFunction &&reduce, CleanUpFunction &&cleanup,
          MapReduceOption option = MapReduceOption::Unordered,
          QThreadPool *pool = nullptr, QThread::Priority priority = QThread::InheritPriority)
{
    return runAsync(priority,
                    Internal::blockingContainerMapReduce<
                        std::decay_t<Container>,
                        std::decay_t<InitFunction>,
                        std::decay_t<MapFunction>,
                        std::decay_t<ReduceResult>,
                        std::decay_t<ReduceFunction>,
                        std::decay_t<CleanUpFunction>>,
                    std::forward<Container>(container),
                    std::forward<InitFunction>(init), std::forward<MapFunction>(map),
                    std::forward<ReduceFunction>(reduce), std::forward<CleanUpFunction>(cleanup),
                    option, pool);
}

template <typename Container, typename InitFunction, typename MapFunction,
          typename ReduceFunction, typename CleanUpFunction,
          typename ReduceResult = typename Internal::resultType<ReduceFunction>::type>
QFuture<ReduceResult>
mapReduce(std::reference_wrapper<Container> containerWrapper, InitFunction &&init, MapFunction &&map,
          ReduceFunction &&reduce, CleanUpFunction &&cleanup,
          MapReduceOption option = MapReduceOption::Unordered,
          QThreadPool *pool = nullptr, QThread::Priority priority = QThread::InheritPriority)
{
    return runAsync(priority,
                    Internal::blockingContainerRefMapReduce<
                        Container,
                        std::decay_t<InitFunction>,
                        std::decay_t<MapFunction>,
                        std::decay_t<ReduceResult>,
                        std::decay_t<ReduceFunction>,
                        std::decay_t<CleanUpFunction>>,
                    containerWrapper,
                    std::forward<InitFunction>(init), std::forward<MapFunction>(map),
                    std::forward<ReduceFunction>(reduce), std::forward<CleanUpFunction>(cleanup),
                    option, pool);
}

template <typename ForwardIterator, typename MapFunction, typename State, typename ReduceFunction,
          typename StateResult = std::decay_t<State>, // State = T& or const T& for lvalues, so decay that away
          typename MapResult = typename Internal::resultType<MapFunction>::type>
QFuture<StateResult>
mapReduce(ForwardIterator begin, ForwardIterator end, MapFunction &&map, State &&initialState,
          ReduceFunction &&reduce, MapReduceOption option = MapReduceOption::Unordered,
          QThreadPool *pool = nullptr, QThread::Priority priority = QThread::InheritPriority,
          int size = -1)
{
    return mapReduce(begin, end,
                     Internal::StateWrapper<State>(std::forward<State>(initialState)),
                     std::forward<MapFunction>(map),
                     Internal::ReduceWrapper<StateResult, MapResult, ReduceFunction>(std::forward<ReduceFunction>(reduce)),
                     &Internal::cleanupReportingState<StateResult>,
                     option, pool, priority, size);
}

template <typename Container, typename MapFunction, typename State, typename ReduceFunction,
          typename StateResult = std::decay_t<State>, // State = T& or const T& for lvalues, so decay that away
          typename MapResult = typename Internal::resultType<MapFunction>::type>
QFuture<StateResult>
mapReduce(Container &&container, MapFunction &&map, State &&initialState, ReduceFunction &&reduce,
          MapReduceOption option = MapReduceOption::Unordered,
          QThreadPool *pool = nullptr, QThread::Priority priority = QThread::InheritPriority)
{
    return mapReduce(std::forward<Container>(container),
                     Internal::StateWrapper<State>(std::forward<State>(initialState)),
                     std::forward<MapFunction>(map),
                     Internal::ReduceWrapper<StateResult, MapResult, ReduceFunction>(std::forward<ReduceFunction>(reduce)),
                     &Internal::cleanupReportingState<StateResult>,
                     option, pool, priority);
}

template <typename ForwardIterator, typename MapFunction, typename State, typename ReduceFunction,
          typename StateResult = std::decay_t<State>, // State = T& or const T& for lvalues, so decay that away
          typename MapResult = typename Internal::resultType<MapFunction>::type>
Q_REQUIRED_RESULT
StateResult
mappedReduced(ForwardIterator begin, ForwardIterator end, MapFunction &&map, State &&initialState,
              ReduceFunction &&reduce, MapReduceOption option = MapReduceOption::Unordered,
              QThreadPool *pool = nullptr, QThread::Priority priority = QThread::InheritPriority,
              int size = -1)
{
    return mapReduce(begin, end,
                     std::forward<MapFunction>(map), std::forward<State>(initialState),
                     std::forward<ReduceFunction>(reduce),
                     option, pool, priority, size).result();
}

template <typename Container, typename MapFunction, typename State, typename ReduceFunction,
          typename StateResult = std::decay_t<State>, // State = T& or const T& for lvalues, so decay that away
          typename MapResult = typename Internal::resultType<MapFunction>::type>
Q_REQUIRED_RESULT
StateResult
mappedReduced(Container &&container, MapFunction &&map, State &&initialState, ReduceFunction &&reduce,
              MapReduceOption option = MapReduceOption::Unordered,
              QThreadPool *pool = nullptr, QThread::Priority priority = QThread::InheritPriority)
{
    return mapReduce(std::forward<Container>(container), std::forward<MapFunction>(map),
                     std::forward<State>(initialState), std::forward<ReduceFunction>(reduce),
                     option, pool, priority).result();
}

template <typename ForwardIterator, typename MapFunction,
          typename MapResult = typename Internal::resultType<MapFunction>::type>
QFuture<MapResult>
map(ForwardIterator begin, ForwardIterator end, MapFunction &&map,
    MapReduceOption option = MapReduceOption::Ordered,
    QThreadPool *pool = nullptr, QThread::Priority priority = QThread::InheritPriority,
    int size = -1)
{
    return mapReduce(begin, end,
                     &Internal::dummyInit<MapResult>,
                     std::forward<MapFunction>(map),
                     Internal::DummyReduce<MapResult>(),
                     &Internal::dummyCleanup<MapResult>,
                     option, pool, priority, size);
}

template <typename Container, typename MapFunction,
          typename MapResult = typename Internal::resultType<MapFunction>::type>
QFuture<MapResult>
map(Container &&container, MapFunction &&map, MapReduceOption option = MapReduceOption::Ordered,
    QThreadPool *pool = nullptr, QThread::Priority priority = QThread::InheritPriority)
{
    return mapReduce(std::forward<Container>(container),
                     Internal::dummyInit<MapResult>,
                     std::forward<MapFunction>(map),
                     Internal::DummyReduce<MapResult>(),
                     Internal::dummyCleanup<MapResult>,
                     option, pool, priority);
}

template <template<typename> class ResultContainer, typename ForwardIterator, typename MapFunction,
          typename MapResult = typename Internal::resultType<MapFunction>::type>
Q_REQUIRED_RESULT
ResultContainer<MapResult>
mapped(ForwardIterator begin, ForwardIterator end, MapFunction &&mapFun,
    MapReduceOption option = MapReduceOption::Ordered,
       QThreadPool *pool = nullptr, QThread::Priority priority = QThread::InheritPriority, int size = -1)
{
    return Utils::transform<ResultContainer>(map(begin, end,
                                                 std::forward<MapFunction>(mapFun),
                                                 option, pool, priority, size).results(),
                                             [](const MapResult &r) { return r; });
}

template <template<typename> class ResultContainer, typename Container, typename MapFunction,
          typename MapResult = typename Internal::resultType<MapFunction>::type>
Q_REQUIRED_RESULT
ResultContainer<MapResult>
mapped(Container &&container, MapFunction &&mapFun,
       MapReduceOption option = MapReduceOption::Ordered,
       QThreadPool *pool = nullptr, QThread::Priority priority = QThread::InheritPriority)
{
    return Utils::transform<ResultContainer>(map(container,
                                                 std::forward<MapFunction>(mapFun),
                                                 option, pool, priority).results(),
                                             [](const MapResult &r) { return r; });
}

} // Utils
