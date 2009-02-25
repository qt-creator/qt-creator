/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef QTCONCURRENT_RUNEX_H
#define QTCONCURRENT_RUNEX_H

#include <qrunnable.h>
#include <qfutureinterface.h>
#include <qthreadpool.h>

QT_BEGIN_NAMESPACE

namespace QtConcurrent {

template <typename T,  typename FunctionPointer>
class StoredInterfaceFunctionCall0 : public QRunnable
{
public:
    StoredInterfaceFunctionCall0(void (fn)(QFutureInterface<T> &))
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
    StoredInterfaceFunctionCall1(void (fn)(QFutureInterface<T> &, Arg1), Arg1 arg1)
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
    StoredInterfaceMemberFunctionCall1(void (Class::*fn)(QFutureInterface<T> &, Arg1), Class *object, Arg1 arg1)
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
    StoredInterfaceFunctionCall2(void (fn)(QFutureInterface<T> &, Arg1, Arg2), Arg1 arg1, Arg2 arg2)
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
    StoredInterfaceMemberFunctionCall2(void (Class::*fn)(QFutureInterface<T> &, Arg1, Arg2), Class *object, Arg1 arg1, Arg2 arg2)
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
    StoredInterfaceFunctionCall3(void (fn)(QFutureInterface<T> &, Arg1, Arg2, Arg3), Arg1 arg1, Arg2 arg2, Arg3 arg3)
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
    StoredInterfaceMemberFunctionCall3(void (Class::*fn)(QFutureInterface<T> &, Arg1, Arg2, Arg3), Class *object, Arg1 arg1, Arg2 arg2, Arg3 arg3)
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
    StoredInterfaceFunctionCall4(void (fn)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
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
    StoredInterfaceMemberFunctionCall4(void (Class::*fn)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4), Class *object, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
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
    StoredInterfaceFunctionCall5(void (fn)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4, Arg5), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
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
    StoredInterfaceMemberFunctionCall5(void (Class::*fn)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4, Arg5), Class *object, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
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
QFuture<T> run(void (*functionPointer)(QFutureInterface<T> &, Arg1), Arg1 arg1)
{
    return (new StoredInterfaceFunctionCall1<T, void (*)(QFutureInterface<T> &, Arg1), Arg1>(functionPointer, arg1))->start();
}
template <typename T, typename Arg1, typename Arg2>
QFuture<T> run(void (*functionPointer)(QFutureInterface<T> &, Arg1, Arg2), Arg1 arg1, Arg2 arg2)
{
    return (new StoredInterfaceFunctionCall2<T, void (*)(QFutureInterface<T> &, Arg1, Arg2), Arg1, Arg2>(functionPointer, arg1, arg2))->start();
}
template <typename T, typename Arg1, typename Arg2, typename Arg3>
QFuture<T> run(void (*functionPointer)(QFutureInterface<T> &, Arg1, Arg2, Arg3), Arg1 arg1, Arg2 arg2, Arg3 arg3)
{
    return (new StoredInterfaceFunctionCall3<T, void (*)(QFutureInterface<T> &, Arg1, Arg2, Arg3), Arg1, Arg2, Arg3>(functionPointer, arg1, arg2, arg3))->start();
}
template <typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
QFuture<T> run(void (*functionPointer)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
{
    return (new StoredInterfaceFunctionCall4<T, void (*)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4), Arg1, Arg2, Arg3, Arg4>(functionPointer, arg1, arg2, arg3, arg4))->start();
}
template <typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
QFuture<T> run(void (*functionPointer)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4, Arg5), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
{
    return (new StoredInterfaceFunctionCall5<T, void (*)(QFutureInterface<T> &, Arg1, Arg2, Arg3, Arg4, Arg5), Arg1, Arg2, Arg3, Arg4, Arg5>(functionPointer, arg1, arg2, arg3, arg4, arg5))->start();
}

template <typename Class, typename T>
QFuture<T> run(void (Class::*fn)(QFutureInterface<T> &), Class *object)
{
    return (new StoredInterfaceMemberFunctionCall0<T, void (Class::*)(QFutureInterface<T> &), Class>(fn, object))->start();
}

} // namespace QtConcurrent

QT_END_NAMESPACE

#endif // QTCONCURRENT_RUNEX_H
