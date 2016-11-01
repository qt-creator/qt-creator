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

#include <utils/runextensions.h>

#include <QtTest>

#if !defined(Q_CC_MSVC) || _MSC_VER >= 1900 // MSVC2015
#define SUPPORTS_MOVE
#endif

class tst_RunExtensions : public QObject
{
    Q_OBJECT

private slots:
    void runAsync();
    void runInThreadPool();
#ifdef SUPPORTS_MOVE
    void moveOnlyType();
#endif
    void threadPriority();
    void runAsyncNoFutureInterface();
    void crefFunction();
    void onResultReady();
};

void report3(QFutureInterface<int> &fi)
{
    fi.reportResults({0, 2, 1});
}

void reportN(QFutureInterface<double> &fi, int n)
{
    fi.reportResults(QVector<double>(n, 0));
}

void reportString1(QFutureInterface<QString> &fi, const QString &s)
{
    fi.reportResult(s);
}

void reportString2(QFutureInterface<QString> &fi, QString s)
{
    fi.reportResult(s);
}

class Callable {
public:
    void operator()(QFutureInterface<double> &fi, int n) const
    {
        fi.reportResults(QVector<double>(n, 0));
    }
};

class MyObject {
public:
    static void staticMember0(QFutureInterface<double> &fi)
    {
        fi.reportResults({0, 2, 1});
    }

    static void staticMember1(QFutureInterface<double> &fi, int n)
    {
        fi.reportResults(QVector<double>(n, 0));
    }

    void member0(QFutureInterface<double> &fi) const
    {
        fi.reportResults({0, 2, 1});
    }

    void member1(QFutureInterface<double> &fi, int n) const
    {
        fi.reportResults(QVector<double>(n, 0));
    }

    void memberString1(QFutureInterface<QString> &fi, const QString &s) const
    {
        fi.reportResult(s);
    }

    void memberString2(QFutureInterface<QString> &fi, QString s) const
    {
        fi.reportResult(s);
    }

    void nonConstMember(QFutureInterface<double> &fi)
    {
        fi.reportResults({0, 2, 1});
    }
};

void voidFunction(bool *value) // can be useful to get QFuture for watching when it is finished
{
    *value = true;
}

int one()
{
    return 1;
}

int identity(int input)
{
    return input;
}

QString stringIdentity1(const QString &s)
{
    return s;
}

QString stringIdentity2(QString s)
{
    return s;
}

class CallableWithoutQFutureInterface {
public:
    void operator()(bool *value) const
    {
        *value = true;
    }
};

class MyObjectWithoutQFutureInterface {
public:
    static void staticMember0(bool *value)
    {
        *value = true;
    }

    static double staticMember1(int n)
    {
        return n;
    }

    void member0(bool *value) const
    {
        *value = true;
    }

    double member1(int n) const
    {
        return n;
    }

    QString memberString1(const QString &s) const
    {
        return s;
    }

    QString memberString2(QString s) const
    {
        return s;
    }

    double nonConstMember(int n)
    {
        return n;
    }
};

void tst_RunExtensions::runAsync()
{
    // free function pointer
    QCOMPARE(Utils::runAsync(&report3).results(),
             QList<int>({0, 2, 1}));
    QCOMPARE(Utils::runAsync(report3).results(),
             QList<int>({0, 2, 1}));

    QCOMPARE(Utils::runAsync(reportN, 4).results(),
             QList<double>({0, 0, 0, 0}));
    QCOMPARE(Utils::runAsync(reportN, 2).results(),
             QList<double>({0, 0}));

    QString s = QLatin1String("string");
    const QString &crs = QLatin1String("cr string");
    const QString cs = QLatin1String("c string");

    QCOMPARE(Utils::runAsync(reportString1, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync(reportString1, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync(reportString1, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync(reportString1, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));

    QCOMPARE(Utils::runAsync(reportString2, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync(reportString2, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync(reportString2, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync(reportString2, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));

    // lambda
    QCOMPARE(Utils::runAsync([](QFutureInterface<double> &fi, int n) {
                 fi.reportResults(QVector<double>(n, 0));
             }, 3).results(),
             QList<double>({0, 0, 0}));

    // std::function
    const std::function<void(QFutureInterface<double>&,int)> fun = [](QFutureInterface<double> &fi, int n) {
        fi.reportResults(QVector<double>(n, 0));
    };
    QCOMPARE(Utils::runAsync(fun, 2).results(),
             QList<double>({0, 0}));

    // operator()
    QCOMPARE(Utils::runAsync(Callable(), 3).results(),
             QList<double>({0, 0, 0}));
    const Callable c{};
    QCOMPARE(Utils::runAsync(c, 2).results(),
             QList<double>({0, 0}));

    // static member functions
    QCOMPARE(Utils::runAsync(&MyObject::staticMember0).results(),
             QList<double>({0, 2, 1}));
    QCOMPARE(Utils::runAsync(&MyObject::staticMember1, 2).results(),
             QList<double>({0, 0}));

    // member functions
    const MyObject obj{};
    QCOMPARE(Utils::runAsync(&MyObject::member0, &obj).results(),
             QList<double>({0, 2, 1}));
    QCOMPARE(Utils::runAsync(&MyObject::member1, &obj, 4).results(),
             QList<double>({0, 0, 0, 0}));
    QCOMPARE(Utils::runAsync(&MyObject::memberString1, &obj, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync(&MyObject::memberString1, &obj, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync(&MyObject::memberString1, &obj, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync(&MyObject::memberString1, &obj, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));
    QCOMPARE(Utils::runAsync(&MyObject::memberString2, &obj, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync(&MyObject::memberString2, &obj, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync(&MyObject::memberString2, &obj, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync(&MyObject::memberString2, &obj, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));
    MyObject nonConstObj{};
    QCOMPARE(Utils::runAsync(&MyObject::nonConstMember, &nonConstObj).results(),
             QList<double>({0, 2, 1}));
}

void tst_RunExtensions::runInThreadPool()
{
    QScopedPointer<QThreadPool> pool(new QThreadPool);
    // free function pointer
    QCOMPARE(Utils::runAsync(pool.data(), &report3).results(),
             QList<int>({0, 2, 1}));
    QCOMPARE(Utils::runAsync(pool.data(), report3).results(),
             QList<int>({0, 2, 1}));

    QCOMPARE(Utils::runAsync(pool.data(), reportN, 4).results(),
             QList<double>({0, 0, 0, 0}));
    QCOMPARE(Utils::runAsync(pool.data(), reportN, 2).results(),
             QList<double>({0, 0}));

    QString s = QLatin1String("string");
    const QString &crs = QLatin1String("cr string");
    const QString cs = QLatin1String("c string");

    QCOMPARE(Utils::runAsync(pool.data(), reportString1, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync(pool.data(), reportString1, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync(pool.data(), reportString1, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync(pool.data(), reportString1, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));

    QCOMPARE(Utils::runAsync(pool.data(), reportString2, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync(pool.data(), reportString2, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync(pool.data(), reportString2, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync(pool.data(), reportString2, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));

    // lambda
    QCOMPARE(Utils::runAsync(pool.data(), [](QFutureInterface<double> &fi, int n) {
                 fi.reportResults(QVector<double>(n, 0));
             }, 3).results(),
             QList<double>({0, 0, 0}));

    // std::function
    const std::function<void(QFutureInterface<double>&,int)> fun = [](QFutureInterface<double> &fi, int n) {
        fi.reportResults(QVector<double>(n, 0));
    };
    QCOMPARE(Utils::runAsync(pool.data(), fun, 2).results(),
             QList<double>({0, 0}));

    // operator()
    QCOMPARE(Utils::runAsync(pool.data(), Callable(), 3).results(),
             QList<double>({0, 0, 0}));
    const Callable c{};
    QCOMPARE(Utils::runAsync(pool.data(), c, 2).results(),
             QList<double>({0, 0}));

    // static member functions
    QCOMPARE(Utils::runAsync(pool.data(), &MyObject::staticMember0).results(),
             QList<double>({0, 2, 1}));
    QCOMPARE(Utils::runAsync(pool.data(), &MyObject::staticMember1, 2).results(),
             QList<double>({0, 0}));

    // member functions
    const MyObject obj{};
    QCOMPARE(Utils::runAsync(pool.data(), &MyObject::member0, &obj).results(),
             QList<double>({0, 2, 1}));
    QCOMPARE(Utils::runAsync(pool.data(), &MyObject::member1, &obj, 4).results(),
             QList<double>({0, 0, 0, 0}));
    QCOMPARE(Utils::runAsync(pool.data(), &MyObject::memberString1, &obj, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync(pool.data(), &MyObject::memberString1, &obj, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync(pool.data(), &MyObject::memberString1, &obj, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync(pool.data(), &MyObject::memberString1, &obj, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));
    QCOMPARE(Utils::runAsync(pool.data(), &MyObject::memberString2, &obj, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync(pool.data(), &MyObject::memberString2, &obj, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync(pool.data(), &MyObject::memberString2, &obj, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync(pool.data(), &MyObject::memberString2, &obj, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));
}

#ifdef SUPPORTS_MOVE

class MoveOnlyType
{
public:
    MoveOnlyType() = default;
    MoveOnlyType(const MoveOnlyType &) = delete;
    MoveOnlyType(MoveOnlyType &&) = default;
    MoveOnlyType &operator=(const MoveOnlyType &) = delete;
    MoveOnlyType &operator=(MoveOnlyType &&) = default;
};

class MoveOnlyCallable : public MoveOnlyType
{
public:
    void operator()(QFutureInterface<int> &fi, const MoveOnlyType &)
    {
        fi.reportResult(1);
    }
};

void tst_RunExtensions::moveOnlyType()
{
    QCOMPARE(Utils::runAsync(MoveOnlyCallable(), MoveOnlyType()).results(),
             QList<int>({1}));
}

#endif

void tst_RunExtensions::threadPriority()
{
    QScopedPointer<QThreadPool> pool(new QThreadPool);
    // with pool
    QCOMPARE(Utils::runAsync(pool.data(), QThread::LowestPriority, &report3).results(),
             QList<int>({0, 2, 1}));

    // without pool
    QCOMPARE(Utils::runAsync(QThread::LowestPriority, report3).results(),
             QList<int>({0, 2, 1}));
}

void tst_RunExtensions::runAsyncNoFutureInterface()
{
    // free function pointer
    bool value = false;
    Utils::runAsync(voidFunction, &value).waitForFinished();
    QCOMPARE(value, true);

    QCOMPARE(Utils::runAsync(one).results(),
             QList<int>({1}));
    QCOMPARE(Utils::runAsync(identity, 5).results(),
             QList<int>({5}));

    QString s = QLatin1String("string");
    const QString &crs = QLatin1String("cr string");
    const QString cs = QLatin1String("c string");

    QCOMPARE(Utils::runAsync(stringIdentity1, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync(stringIdentity1, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync(stringIdentity1, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync(stringIdentity1, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));
    QCOMPARE(Utils::runAsync(stringIdentity2, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync(stringIdentity2, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync(stringIdentity2, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync(stringIdentity2, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));

    // lambda
    QCOMPARE(Utils::runAsync([](int n) -> double {
                 return n + 1;
             }, 3).results(),
             QList<double>({4}));

    // std::function
    const std::function<double(int)> fun = [](int n) {
        return n + 1;
    };
    QCOMPARE(Utils::runAsync(fun, 2).results(),
             QList<double>({3}));

    // operator()
    value = false;
    Utils::runAsync(CallableWithoutQFutureInterface(), &value).waitForFinished();
    QCOMPARE(value, true);
    value = false;
    const CallableWithoutQFutureInterface c{};
    Utils::runAsync(c, &value).waitForFinished();
    QCOMPARE(value, true);

    // static member functions
    value = false;
    Utils::runAsync(&MyObjectWithoutQFutureInterface::staticMember0, &value).waitForFinished();
    QCOMPARE(value, true);
    QCOMPARE(Utils::runAsync(&MyObjectWithoutQFutureInterface::staticMember1, 2).results(),
             QList<double>({2}));

    // member functions
    const MyObjectWithoutQFutureInterface obj{};
    value = false;
    Utils::runAsync(&MyObjectWithoutQFutureInterface::member0, &obj, &value).waitForFinished();
    QCOMPARE(value, true);
    QCOMPARE(Utils::runAsync(&MyObjectWithoutQFutureInterface::member1, &obj, 4).results(),
             QList<double>({4}));
    QCOMPARE(Utils::runAsync(&MyObjectWithoutQFutureInterface::memberString1, &obj, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync(&MyObjectWithoutQFutureInterface::memberString1, &obj, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync(&MyObjectWithoutQFutureInterface::memberString1, &obj, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync(&MyObjectWithoutQFutureInterface::memberString1, &obj, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));
    QCOMPARE(Utils::runAsync(&MyObjectWithoutQFutureInterface::memberString2, &obj, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync(&MyObjectWithoutQFutureInterface::memberString2, &obj, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync(&MyObjectWithoutQFutureInterface::memberString2, &obj, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync(&MyObjectWithoutQFutureInterface::memberString2, &obj, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));
    MyObjectWithoutQFutureInterface nonConstObj{};
    QCOMPARE(Utils::runAsync(&MyObjectWithoutQFutureInterface::nonConstMember, &nonConstObj, 4).results(),
             QList<double>({4}));
}

void tst_RunExtensions::crefFunction()
{
    // free function pointer with future interface
    auto fun = &report3;
    QCOMPARE(Utils::runAsync(std::cref(fun)).results(),
             QList<int>({0, 2, 1}));

    // lambda with future interface
    auto lambda = [](QFutureInterface<double> &fi, int n) {
        fi.reportResults(QVector<double>(n, 0));
    };
    QCOMPARE(Utils::runAsync(std::cref(lambda), 3).results(),
             QList<double>({0, 0, 0}));

    // std::function with future interface
    const std::function<void(QFutureInterface<double>&,int)> funObj = [](QFutureInterface<double> &fi, int n) {
        fi.reportResults(QVector<double>(n, 0));
    };
    QCOMPARE(Utils::runAsync(std::cref(funObj), 2).results(),
             QList<double>({0, 0}));

    // callable with future interface
    const Callable c{};
    QCOMPARE(Utils::runAsync(std::cref(c), 2).results(),
             QList<double>({0, 0}));

    // member functions with future interface
    auto member = &MyObject::member0;
    const MyObject obj{};
    QCOMPARE(Utils::runAsync(std::cref(member), &obj).results(),
             QList<double>({0, 2, 1}));

    // free function pointer without future interface
    bool value = false;
    auto voidFun = &voidFunction;
    Utils::runAsync(std::cref(voidFun), &value).waitForFinished();
    QCOMPARE(value, true);

    auto oneFun = &one;
    QCOMPARE(Utils::runAsync(std::cref(oneFun)).results(),
             QList<int>({1}));

    // lambda without future interface
    auto lambda2 = [](int n) -> double {
        return n + 1;
    };
    QCOMPARE(Utils::runAsync(std::cref(lambda2), 3).results(),
             QList<double>({4}));

    // std::function
    const std::function<double(int)> funObj2 = [](int n) {
        return n + 1;
    };
    QCOMPARE(Utils::runAsync(std::cref(funObj2), 2).results(),
             QList<double>({3}));

    // callable without future interface
    const CallableWithoutQFutureInterface c2{};
    Utils::runAsync(std::cref(c2), &value).waitForFinished();
    QCOMPARE(value, true);

    // member functions without future interface
    const MyObjectWithoutQFutureInterface obj2{};
    auto member2 = &MyObjectWithoutQFutureInterface::member0;
    value = false;
    Utils::runAsync(std::cref(member2), &obj2, &value).waitForFinished();
    QCOMPARE(value, true);
}

class ObjWithProperty : public QObject
{
    Q_OBJECT

public slots:
    void setValue(const QString &s)
    {
        value = s;
    }

public:
    QString value;
};

void tst_RunExtensions::onResultReady()
{
    { // lambda
        QFuture<QString> f = Utils::runAsync([](QFutureInterface<QString> &fi) {
                fi.reportResult("Hi");
                fi.reportResult("there");
        });
        int count = 0;
        QString res;
        Utils::onResultReady(f, [&count, &res](const QString &s) {
            ++count;
            res = s;
        });
        f.waitForFinished();
        QCoreApplication::processEvents();
        QCOMPARE(count, 2);
        QCOMPARE(res, QString("there"));
    }
    { // lambda with guard
        QFuture<QString> f = Utils::runAsync([](QFutureInterface<QString> &fi) {
                fi.reportResult("Hi");
                fi.reportResult("there");
        });
        int count = 0;
        ObjWithProperty obj;
        Utils::onResultReady(f, &obj, [&count, &obj](const QString &s) {
            ++count;
            obj.setValue(s);
        });
        f.waitForFinished();
        QCoreApplication::processEvents();
        QCOMPARE(count, 2);
        QCOMPARE(obj.value, QString("there"));
    }
    { // member
        QFuture<QString> f = Utils::runAsync([]() { return QString("Hi"); });
        ObjWithProperty obj;
        Utils::onResultReady(f, &obj, &ObjWithProperty::setValue);
        f.waitForFinished();
        QCoreApplication::processEvents();
        QCOMPARE(obj.value, QString("Hi"));
    }
}

QTEST_MAIN(tst_RunExtensions)

#include "tst_runextensions.moc"
