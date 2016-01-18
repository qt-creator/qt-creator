/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef RUNEXTENSIONS_H
#define RUNEXTENSIONS_H

#include "qtcassert.h"

#include <qrunnable.h>
#include <qfuture.h>
#include <qfutureinterface.h>
#include <qthreadpool.h>

#include <chrono>
#include <functional>
#include <future>
#include <thread>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtConcurrent {

template <typename T,  typename FunctionPointer>
class StoredInterfaceFunctionCall0 : public QRunnable
{
public:
    StoredInterfaceFunctionCall0(const FunctionPointer &fn)
    : fn(fn) { }

    QFuture<T> start()
    {
        futureInterface.reportStarted();
        QFuture<T> future = futureInterface.future();
        QThreadPool::globalInstance()->start(this);
        return future;
    }

    void run()
    {
        fn(futureInterface);
        futureInterface.reportFinished();
    }
private:
    QFutureInterface<T> futureInterface;
    FunctionPointer fn;

};
template <typename T,  typename FunctionPointer, typename Class>
class StoredInterfaceMemberFunctionCall0 : public QRunnable
{
public:
    StoredInterfaceMemberFunctionCall0(void (Class::*fn)(QFutureInterface<T> &), Class *object)
    : fn(fn), object(object) { }

    QFuture<T> start()
    {
        futureInterface.reportStarted();
        QFuture<T> future = futureInterface.future();
        QThreadPool::globalInstance()->start(this);
        return future;
    }

    void run()
    {
        (object->*fn)(futureInterface);
        futureInterface.reportFinished();
    }
private:
    QFutureInterface<T> futureInterface;
    FunctionPointer fn;
    Class *object;

};

template <typename T,  typename FunctionPointer, typename Arg1>
class StoredInterfaceFunctionCall1 : public QRunnable
{
public:
    StoredInterfaceFunctionCall1(void (fn)(QFutureInterface<T> &, Arg1), const Arg1 &arg1)
    : fn(fn), arg1(arg1) { }

    QFuture<T> start()
    {
        futureInterface.reportStarted();
        QFuture<T> future = futureInterface.future();
        QThreadPool::globalInstance()->start(this);
        return future;
    }

    void run()
    {
        fn(futureInterface, arg1);
        futureInterface.reportFinished();
    }
private:
    QFutureInterface<T> futureInterface;
    FunctionPointer fn;
    Arg1 arg1;
};
template <typename T,  typename FunctionPointer, typename Class, typename Arg1>
class StoredInterfaceMemberFunctionCall1 : public QRunnable
{
public:
    StoredInterfaceMemberFunctionCall1(void (Class::*fn)(QFutureInterface<T> &, Arg1), Class *object, const Arg1 &arg1)
    : fn(fn), object(object), arg1(arg1) { }

    QFuture<T> start()
    {
        futureInterface.reportStarted();
        QFuture<T> future = futureInterface.future();
        QThreadPool::globalInstance()->start(this);
        return future;
    }

    void run()
    {
        (object->*fn)(futureInterface, arg1);
        futureInterface.reportFinished();
    }
private:
    QFutureInterface<T> futureInterface;
    FunctionPointer fn;
    Class *object;
    Arg1 arg1;
};

template <typename T,  typename FunctionPointer, typename Arg1, typename Arg2>
class StoredInterfaceFunctionCall2 : public QRunnable
{
public:
    StoredInterfaceFunctionCall2(void (fn)(QFutureInterface<T> &, Arg1, Arg2), const Arg1 &arg1, const Arg2 &arg2)
    : fn(fn), arg1(arg1), arg2(arg2) { }

    QFuture<T> start()
    {
        futureInterface.reportStarted();
        QFuture<T> future = futureInterface.future();
        QThreadPool::globalInstance()->start(this);
        return future;
    }

    void run()
    {
        fn(futureInterface, arg1, arg2);
        futureInterface.reportFinished();
    }
private:
    QFutureInterface<T> futureInterface;
    FunctionPointer fn;
    Arg1 arg1; Arg2 arg2;
};
template <typename T,  typename FunctionPointer, typename Class, typename Arg1, typename Arg2>
class StoredInterfaceMemberFunctionCall2 : public QRunnable
{
public:
    StoredInterfaceMemberFunctionCall2(void (Class::*fn)(QFutureInterface<T> &, Arg1, Arg2), Class *object, const Arg1 &arg1, const Arg2 &arg2)
    : fn(fn), object(object), arg1(arg1), arg2(arg2) { }

    QFuture<T> start()
    {
        futureInterface.reportStarted();
        QFuture<T> future = futureInterface.future();
        QThreadPool::globalInstance()->start(this);
        return future;
    }

    void run()
    {
        (object->*fn)(futureInterface, arg1, arg2);
        futureInterface.reportFinished();
    }
private:
    QFutureInterface<T> futureInterface;
    FunctionPointer fn;
    Class *object;
    Arg1 arg1; Arg2 arg2;
};

template <typename T,  typename FunctionPointer, typename Arg1, typename Arg2, typename Arg3>
class StoredInterfaceFunctionCall3 : public QRunnable
{
public:
    StoredInterfaceFunctionCall3(void (fn)(QFutureInterface<T> &, Arg1, Arg2, Arg3), const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3)
    : fn(fn), arg1(arg1), arg2(arg2), arg3(arg3) { }

    QFuture<T> start()
    {
        futureInterface.reportStarted();
        QFuture<T> future = futureInterface.future();
        QThreadPool::globalInstance()->start(this);
        return future;
    }

    void run()
    {
        fn(futureInterface, arg1, arg2, arg3);
        futureInterface.reportFinished();
    }
private:
    QFutureInterface<T> futureInterface;
    FunctionPointer fn;
    Arg1 arg1; Arg2 arg2; Arg3 arg3;
};
template <typename T,  typename FunctionPointer, typename Class, typename Arg1, typename Arg2, typename Arg3>
class StoredInterfaceMemberFunctionCall3 : public QRunnable
{
public:
    StoredInterfaceMemberFunctionCall3(void (Class::*fn)(QFutureInterface<T> &, Arg1, Arg2, Arg3), Class *object, const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3)
    : fn(fn), object(object), arg1(arg1), arg2(arg2), arg3(arg3) { }

    QFuture<T> start()
    {
        futureInterface.reportStarted();
        QFuture<T> future = futureInterface.future();
        QThreadPool::globalInstance()->start(this);
        return future;
    }

    void run()
    {
        (object->*fn)(futureInterface, arg1, arg2, arg3);
        futureInterface.reportFinished();
    }
private:
    QFutureInterface<T> futureInterface;
    FunctionPointer fn;
    Class *object;
    Arg1 arg1; Arg2 arg2; Arg3 arg3;
};

template <typename T,  typename FunctionPointer, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
class StoredInterfaceFunctionCall4 : public QRunnable
{
public:
    StoredInterfaceFunctionCall4(void (fn)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4), const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3, const Arg4 &arg4)
    : fn(fn), arg1(arg1), arg2(arg2), arg3(arg3), arg4(arg4) { }

    QFuture<T> start()
    {
        futureInterface.reportStarted();
        QFuture<T> future = futureInterface.future();
        QThreadPool::globalInstance()->start(this);
        return future;
    }

    void run()
    {
        fn(futureInterface, arg1, arg2, arg3, arg4);
        futureInterface.reportFinished();
    }
private:
    QFutureInterface<T> futureInterface;
    FunctionPointer fn;
    Arg1 arg1; Arg2 arg2; Arg3 arg3; Arg4 arg4;
};
template <typename T,  typename FunctionPointer, typename Class, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
class StoredInterfaceMemberFunctionCall4 : public QRunnable
{
public:
    StoredInterfaceMemberFunctionCall4(void (Class::*fn)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4), Class *object, const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3, const Arg4 &arg4)
    : fn(fn), object(object), arg1(arg1), arg2(arg2), arg3(arg3), arg4(arg4) { }

    QFuture<T> start()
    {
        futureInterface.reportStarted();
        QFuture<T> future = futureInterface.future();
        QThreadPool::globalInstance()->start(this);
        return future;
    }

    void run()
    {
        (object->*fn)(futureInterface, arg1, arg2, arg3, arg4);
        futureInterface.reportFinished();
    }
private:
    QFutureInterface<T> futureInterface;
    FunctionPointer fn;
    Class *object;
    Arg1 arg1; Arg2 arg2; Arg3 arg3; Arg4 arg4;
};

template <typename T,  typename FunctionPointer, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
class StoredInterfaceFunctionCall5 : public QRunnable
{
public:
    StoredInterfaceFunctionCall5(void (fn)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4, Arg5), const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3, const Arg4 &arg4, const Arg5 &arg5)
    : fn(fn), arg1(arg1), arg2(arg2), arg3(arg3), arg4(arg4), arg5(arg5) { }

    QFuture<T> start()
    {
        futureInterface.reportStarted();
        QFuture<T> future = futureInterface.future();
        QThreadPool::globalInstance()->start(this);
        return future;
    }

    void run()
    {
        fn(futureInterface, arg1, arg2, arg3, arg4, arg5);
        futureInterface.reportFinished();
    }
private:
    QFutureInterface<T> futureInterface;
    FunctionPointer fn;
    Arg1 arg1; Arg2 arg2; Arg3 arg3; Arg4 arg4; Arg5 arg5;
};
template <typename T,  typename FunctionPointer, typename Class, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
class StoredInterfaceMemberFunctionCall5 : public QRunnable
{
public:
    StoredInterfaceMemberFunctionCall5(void (Class::*fn)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4, Arg5), Class *object, const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3, const Arg4 &arg4, const Arg5 &arg5)
    : fn(fn), object(object), arg1(arg1), arg2(arg2), arg3(arg3), arg4(arg4), arg5(arg5) { }

    QFuture<T> start()
    {
        futureInterface.reportStarted();
        QFuture<T> future = futureInterface.future();
        QThreadPool::globalInstance()->start(this);
        return future;
    }

    void run()
    {
        (object->*fn)(futureInterface, arg1, arg2, arg3, arg4, arg5);
        futureInterface.reportFinished();
    }
private:
    QFutureInterface<T> futureInterface;
    FunctionPointer fn;
    Class *object;
    Arg1 arg1; Arg2 arg2; Arg3 arg3; Arg4 arg4; Arg5 arg5;
};

template <typename T>
QFuture<T> run(void (*functionPointer)(QFutureInterface<T> &))
{
    return (new StoredInterfaceFunctionCall0<T, void (*)(QFutureInterface<T> &)>(functionPointer))->start();
}
template <typename T, typename Arg1>
QFuture<T> run(void (*functionPointer)(QFutureInterface<T> &, Arg1), const Arg1 &arg1)
{
    return (new StoredInterfaceFunctionCall1<T, void (*)(QFutureInterface<T> &, Arg1), Arg1>(functionPointer, arg1))->start();
}
template <typename T, typename Arg1, typename Arg2>
QFuture<T> run(void (*functionPointer)(QFutureInterface<T> &, Arg1, Arg2), const Arg1 &arg1, const Arg2 &arg2)
{
    return (new StoredInterfaceFunctionCall2<T, void (*)(QFutureInterface<T> &, Arg1, Arg2), Arg1, Arg2>(functionPointer, arg1, arg2))->start();
}
template <typename T, typename Arg1, typename Arg2, typename Arg3>
QFuture<T> run(void (*functionPointer)(QFutureInterface<T> &, Arg1, Arg2, Arg3), const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3)
{
    return (new StoredInterfaceFunctionCall3<T, void (*)(QFutureInterface<T> &, Arg1, Arg2, Arg3), Arg1, Arg2, Arg3>(functionPointer, arg1, arg2, arg3))->start();
}
template <typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
QFuture<T> run(void (*functionPointer)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4), const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3, const Arg4 &arg4)
{
    return (new StoredInterfaceFunctionCall4<T, void (*)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4), Arg1, Arg2, Arg3, Arg4>(functionPointer, arg1, arg2, arg3, arg4))->start();
}
template <typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
QFuture<T> run(void (*functionPointer)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4, Arg5), const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3, const Arg4 &arg4, const Arg5 &arg5)
{
    return (new StoredInterfaceFunctionCall5<T, void (*)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4, Arg5), Arg1, Arg2, Arg3, Arg4, Arg5>(functionPointer, arg1, arg2, arg3, arg4, arg5))->start();
}

template <typename Class, typename T>
QFuture<T> run(void (Class::*fn)(QFutureInterface<T> &), Class *object)
{
    return (new StoredInterfaceMemberFunctionCall0<T, void (Class::*)(QFutureInterface<T> &), Class>(fn, object))->start();
}

template <typename Class, typename T, typename Arg1>
QFuture<T> run(void (Class::*fn)(QFutureInterface<T> &, Arg1), Class *object, Arg1 arg1)
{
    return (new StoredInterfaceMemberFunctionCall1<T, void (Class::*)(QFutureInterface<T> &, Arg1), Class, Arg1>(fn, object, arg1))->start();
}

template <typename Class, typename T, typename Arg1, typename Arg2>
QFuture<T> run(void (Class::*fn)(QFutureInterface<T> &, Arg1, Arg2), Class *object, const Arg1 &arg1, const Arg2 &arg2)
{
    return (new StoredInterfaceMemberFunctionCall2<T, void (Class::*)(QFutureInterface<T> &, Arg1, Arg2), Class, Arg1, Arg2>(fn, object, arg1, arg2))->start();
}

template <typename Class, typename T, typename Arg1, typename Arg2, typename Arg3>
QFuture<T> run(void (Class::*fn)(QFutureInterface<T> &, Arg1, Arg2, Arg3), Class *object, const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3)
{
    return (new StoredInterfaceMemberFunctionCall3<T, void (Class::*)(QFutureInterface<T> &, Arg1, Arg2, Arg3), Class, Arg1, Arg2, Arg3>(fn, object, arg1, arg2, arg3))->start();
}

template <typename Class, typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
QFuture<T> run(void (Class::*fn)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4), Class *object, const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3, const Arg4 &arg4)
{
    return (new StoredInterfaceMemberFunctionCall4<T, void (Class::*)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4), Class, Arg1, Arg2, Arg3, Arg4>(fn, object, arg1, arg2, arg3, arg4))->start();
}

template <typename Class, typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
QFuture<T> run(void (Class::*fn)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4, Arg5), Class *object, const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3, const Arg4 &arg4, const Arg5 &arg5)
{
    return (new StoredInterfaceMemberFunctionCall5<T, void (Class::*)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4, Arg5), Class, Arg1, Arg2, Arg3, Arg4, Arg5>(fn, object, arg1, arg2, arg3, arg4, arg5))->start();
}

template <typename T>
QFuture<T> run(const std::function<void (QFutureInterface<T> &)> &fn)
{
    return (new StoredInterfaceFunctionCall0<T, std::function<void (QFutureInterface<T> &)>>(fn))->start();
}

} // namespace QtConcurrent

QT_END_NAMESPACE

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

// TODO: Use universal references and std::forward to function when we require MSVC2015
// (MSVC2013 has a bug that std::thread does not move the copied arguments to the function call,
// which leads to compile errors with rvalue arguments)
template <typename ResultType, typename Function, typename Obj, typename... Args>
typename std::enable_if<std::is_member_pointer<typename std::decay<Function>::type>::value>::type
runAsyncImpl(QFutureInterface<ResultType> futureInterface, const Function &function, const Obj &obj, const Args&... args)
{
    std::mem_fn(function)(obj, futureInterface, args...);
    if (futureInterface.isPaused())
        futureInterface.waitForResume();
    futureInterface.reportFinished();
}

// TODO: Use universal references and std::forward to function when we require MSVC2015
// (MSVC2013 has a bug that std::thread does not move the copied arguments to the function call,
// which leads to compile errors with rvalue arguments)
template <typename ResultType, typename Function, typename... Args>
typename std::enable_if<!std::is_member_pointer<typename std::decay<Function>::type>::value>::type
runAsyncImpl(QFutureInterface<ResultType> futureInterface, const Function &function, const Args&... args)
{
    function(futureInterface, args...);
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

/*!
    The interface of \c {runAsync} is similar to the std::thread constructor and \c {std::invoke}.

    The \a function argument can be a member function, an object with \c {operator()}, a
    \c {std::function}, lambda, function pointer or function reference.
    They need to take a \c {QFutureInterface<ResultType>&} as their first argument, followed by
    other custom arguments which need to be passed to this function.
    If \a function is a (non-static) member function, the first argument in \a args is expected
    to be the object that the function is called on.

    \sa std::thread
    \sa std::invoke
 */
template <typename ResultType, typename Function, typename... Args>
QFuture<ResultType> runAsync(Function &&function, Args&&... args)
{
    QFutureInterface<ResultType> futureInterface;
    futureInterface.reportStarted();
    std::thread(Internal::runAsyncImpl<ResultType,typename std::decay<Function>::type,typename std::decay<Args>::type...>,
                futureInterface, std::forward<Function>(function), std::forward<Args>(args)...).detach();
    return futureInterface.future();
}

} // Utils

#endif // RUNEXTENSIONS_H
