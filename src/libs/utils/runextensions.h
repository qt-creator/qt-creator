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

#ifndef RUNEXTENSIONS_H
#define RUNEXTENSIONS_H

#include "functiontraits.h"
#include "qtcassert.h"
#include "utils_global.h"

#include <QCoreApplication>
#include <QFuture>
#include <QFutureInterface>
#include <QRunnable>
#include <QThread>
#include <QThreadPool>

#include <functional>

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
namespace Internal {

template <typename Function>
class MemberCallable;

template <typename Result, typename Obj, typename... Args>
class MemberCallable<Result(Obj::*)(Args...) const>
{
public:
    MemberCallable(Result(Obj::* function)(Args...) const, const Obj *obj)
        : m_function(function),
          m_obj(obj)
    {
    }

    Result operator()(Args&&... args) const
    {
        return ((*m_obj).*m_function)(std::forward<Args>(args)...);
    }

private:
    Result(Obj::* m_function)(Args...) const;
    const Obj *m_obj;
};

template <typename Result, typename Obj, typename... Args>
class MemberCallable<Result(Obj::*)(Args...)>
{
public:
    MemberCallable(Result(Obj::* function)(Args...), Obj *obj)
        : m_function(function),
          m_obj(obj)
    {
    }

    Result operator()(Args&&... args) const
    {
        return ((*m_obj).*m_function)(std::forward<Args>(args)...);
    }

private:
    Result(Obj::* m_function)(Args...);
    Obj *m_obj;
};

// void function that does not take QFutureInterface
template <typename ResultType, typename Function, typename... Args>
void runAsyncReturnVoidDispatch(std::true_type, QFutureInterface<ResultType> futureInterface, Function &&function, Args&&... args)
{
    function(std::forward<Args>(args)...);
    if (futureInterface.isPaused())
        futureInterface.waitForResume();
    futureInterface.reportFinished();
}

// non-void function that does not take QFutureInterface
template <typename ResultType, typename Function, typename... Args>
void runAsyncReturnVoidDispatch(std::false_type, QFutureInterface<ResultType> futureInterface, Function &&function, Args&&... args)
{
    futureInterface.reportResult(function(std::forward<Args>(args)...));
    if (futureInterface.isPaused())
        futureInterface.waitForResume();
    futureInterface.reportFinished();
}

// function that takes QFutureInterface
template <typename ResultType, typename Function, typename... Args>
void runAsyncQFutureInterfaceDispatch(std::true_type, QFutureInterface<ResultType> futureInterface, Function &&function, Args&&... args)
{
    function(futureInterface, std::forward<Args>(args)...);
    if (futureInterface.isPaused())
        futureInterface.waitForResume();
    futureInterface.reportFinished();
}

// function that does not take QFutureInterface
template <typename ResultType, typename Function, typename... Args>
void runAsyncQFutureInterfaceDispatch(std::false_type, QFutureInterface<ResultType> futureInterface, Function &&function, Args&&... args)
{
    runAsyncReturnVoidDispatch(std::is_void<typename std::result_of<Function(Args...)>::type>(),
                               futureInterface, std::forward<Function>(function), std::forward<Args>(args)...);
}

// function that takes at least one argument which could be QFutureInterface
template <typename ResultType, typename Function, typename... Args>
void runAsyncArityDispatch(std::true_type, QFutureInterface<ResultType> futureInterface, Function &&function, Args&&... args)
{
    runAsyncQFutureInterfaceDispatch(std::is_same<QFutureInterface<ResultType>&,
                                                  typename functionTraits<Function>::template argument<0>::type>(),
                                     futureInterface, std::forward<Function>(function), std::forward<Args>(args)...);
}

// function that does not take an argument, so it does not take a QFutureInterface
template <typename ResultType, typename Function, typename... Args>
void runAsyncArityDispatch(std::false_type, QFutureInterface<ResultType> futureInterface, Function &&function, Args&&... args)
{
    runAsyncQFutureInterfaceDispatch(std::false_type(),
                                     futureInterface, std::forward<Function>(function), std::forward<Args>(args)...);
}

// function, function pointer, or other callable object that is no member pointer
template <typename ResultType, typename Function, typename... Args,
          typename = typename std::enable_if<
                !std::is_member_pointer<typename std::decay<Function>::type>::value
              >::type>
void runAsyncImpl(QFutureInterface<ResultType> futureInterface, Function &&function, Args&&... args)
{
    runAsyncArityDispatch(std::integral_constant<bool, (functionTraits<Function>::arity > 0)>(),
                          futureInterface, std::forward<Function>(function), std::forward<Args>(args)...);
}

// Function = member function
template <typename ResultType, typename Function, typename Obj, typename... Args,
          typename = typename std::enable_if<
                std::is_member_pointer<typename std::decay<Function>::type>::value
              >::type>
void runAsyncImpl(QFutureInterface<ResultType> futureInterface, Function &&function, Obj &&obj, Args&&... args)
{
    // Wrap member function with object into callable
    runAsyncImpl(futureInterface,
                 MemberCallable<Function>(std::forward<Function>(function), std::forward<Obj>(obj)),
                 std::forward<Args>(args)...);
}

// can be replaced with std::(make_)index_sequence with C++14
template <std::size_t...>
struct indexSequence { };
template <std::size_t N, std::size_t... S>
struct makeIndexSequence : makeIndexSequence<N-1, N-1, S...> { };
template <std::size_t... S>
struct makeIndexSequence<0, S...> { typedef indexSequence<S...> type; };

template <class T>
typename std::decay<T>::type
decayCopy(T&& v)
{
    return std::forward<T>(v);
}

template <typename ResultType, typename Function, typename... Args>
class AsyncJob : public QRunnable
{
public:
    AsyncJob(Function &&function, Args&&... args)
          // decay copy like std::thread
        : data(decayCopy(std::forward<Function>(function)), decayCopy(std::forward<Args>(args))...)
    {
        // we need to report it as started even though it isn't yet, because someone might
        // call waitForFinished on the future, which does _not_ block if the future is not started
        futureInterface.setRunnable(this);
        futureInterface.reportStarted();
    }

    ~AsyncJob()
    {
        // QThreadPool can delete runnables even if they were never run (e.g. QThreadPool::clear).
        // Since we reported them as started, we make sure that we always report them as finished.
        // reportFinished only actually sends the signal if it wasn't already finished.
        futureInterface.reportFinished();
    }

    QFuture<ResultType> future() { return futureInterface.future(); }

    void run() override
    {
        if (priority != QThread::InheritPriority)
            if (QThread *thread = QThread::currentThread())
                if (thread != qApp->thread())
                    thread->setPriority(priority);
        if (futureInterface.isCanceled()) {
            futureInterface.reportFinished();
            return;
        }
        runHelper(typename makeIndexSequence<std::tuple_size<Data>::value>::type());
    }

    void setThreadPool(QThreadPool *pool)
    {
        futureInterface.setThreadPool(pool);
    }

    void setThreadPriority(QThread::Priority p)
    {
        priority = p;
    }

private:
    using Data = std::tuple<typename std::decay<Function>::type, typename std::decay<Args>::type...>;

    template <std::size_t... index>
    void runHelper(indexSequence<index...>)
    {
        // invalidates data, which is moved into the call
        runAsyncImpl(futureInterface, std::move(std::get<index>(data))...);
    }

    Data data;
    QFutureInterface<ResultType> futureInterface;
    QThread::Priority priority = QThread::InheritPriority;
};

class QTCREATOR_UTILS_EXPORT RunnableThread : public QThread
{
public:
    explicit RunnableThread(QRunnable *runnable, QObject *parent = 0);

protected:
    void run();

private:
    QRunnable *m_runnable;
};

} // Internal

/*!
    The interface of \c {runAsync} is similar to the std::thread constructor and \c {std::invoke}.

    The \a function argument can be a member function,
    an object with \c {operator()} (with no overloads),
    a \c {std::function}, lambda, function pointer or function reference.
    The \a args are passed to the function call after they are copied/moved to the thread.

    The \a function can take a \c {QFutureInterface<ResultType>&} as its first argument, followed by
    other custom arguments which need to be passed to this function.
    If it does not take a \c {QFutureInterface<ResultType>&} as its first argument
    and its return type is not void, the function call's result is reported to the QFuture.
    If \a function is a (non-static) member function, the first argument in \a args is expected
    to be the object that the function is called on.

    If a thread \a pool is given, the function is run there. Otherwise a new, independent thread
    is started.

    \sa std::thread
    \sa std::invoke
    \sa QThreadPool
    \sa QThread::Priority
 */
template <typename ResultType, typename Function, typename... Args>
QFuture<ResultType> runAsync(QThreadPool *pool, QThread::Priority priority,
                             Function &&function, Args&&... args)
{
    auto job = new Internal::AsyncJob<ResultType,Function,Args...>
            (std::forward<Function>(function), std::forward<Args>(args)...);
    job->setThreadPriority(priority);
    QFuture<ResultType> future = job->future();
    if (pool) {
        job->setThreadPool(pool);
        pool->start(job);
    } else {
        auto thread = new Internal::RunnableThread(job);
        thread->moveToThread(qApp->thread()); // make sure thread gets deleteLater on main thread
        QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
        thread->start();
    }
    return future;
}

/*!
    Runs \a function with \a args in a new thread with given thread \a priority.
    \sa runAsync(QThreadPool*,QThread::Priority,Function&&,Args&&...)
    \sa QThread::Priority
 */
template <typename ResultType, typename Function, typename... Args>
QFuture<ResultType> runAsync(QThread::Priority priority, Function &&function, Args&&... args)
{
    return runAsync<ResultType>(static_cast<QThreadPool *>(nullptr), priority,
                                std::forward<Function>(function), std::forward<Args>(args)...);
}

/*!
    Runs \a function with \a args in a new thread with thread priority QThread::InheritPriority.
    \sa runAsync(QThreadPool*,QThread::Priority,Function&&,Args&&...)
    \sa QThread::Priority
 */
template <typename ResultType, typename Function, typename... Args,
          typename = typename std::enable_if<
                !std::is_same<typename std::decay<Function>::type, QThreadPool>::value
                && !std::is_same<typename std::decay<Function>::type, QThread::Priority>::value
              >::type>
QFuture<ResultType> runAsync(Function &&function, Args&&... args)
{
    return runAsync<ResultType>(static_cast<QThreadPool *>(nullptr),
                                QThread::InheritPriority, std::forward<Function>(function),
                                std::forward<Args>(args)...);
}

/*!
    Runs \a function with \a args in a thread \a pool with thread priority QThread::InheritPriority.
    \sa runAsync(QThreadPool*,QThread::Priority,Function&&,Args&&...)
    \sa QThread::Priority
 */
template <typename ResultType, typename Function, typename... Args,
          typename = typename std::enable_if<
                !std::is_same<typename std::decay<Function>::type, QThread::Priority>::value
              >::type>
QFuture<ResultType> runAsync(QThreadPool *pool,
                             Function &&function, Args&&... args)
{
    return runAsync<ResultType>(pool, QThread::InheritPriority, std::forward<Function>(function),
                                std::forward<Args>(args)...);
}

} // Utils

#endif // RUNEXTENSIONS_H
