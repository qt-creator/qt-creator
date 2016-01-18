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

#include <utils/runextensions.h>

#include <QtTest>

class tst_RunExtensions : public QObject
{
    Q_OBJECT

private slots:
    void runAsync();
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
};

void tst_RunExtensions::runAsync()
{
    // free function pointer
    QCOMPARE(Utils::runAsync<int>(&report3).results(),
             QList<int>({0, 2, 1}));
    QCOMPARE(Utils::runAsync<int>(report3).results(),
             QList<int>({0, 2, 1}));

    QCOMPARE(Utils::runAsync<double>(reportN, 4).results(),
             QList<double>({0, 0, 0, 0}));
    QCOMPARE(Utils::runAsync<double>(reportN, 2).results(),
             QList<double>({0, 0}));

    QString s = QLatin1String("string");
    const QString &crs = QLatin1String("cr string");
    const QString cs = QLatin1String("c string");

    QCOMPARE(Utils::runAsync<QString>(reportString1, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync<QString>(reportString1, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync<QString>(reportString1, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync<QString>(reportString1, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));

    QCOMPARE(Utils::runAsync<QString>(reportString2, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync<QString>(reportString2, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync<QString>(reportString2, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync<QString>(reportString2, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));

    // lambda
    QCOMPARE(Utils::runAsync<double>([](QFutureInterface<double> &fi, int n) {
                 fi.reportResults(QVector<double>(n, 0));
             }, 3).results(),
             QList<double>({0, 0, 0}));

    // std::function
    const std::function<void(QFutureInterface<double>&,int)> fun = [](QFutureInterface<double> &fi, int n) {
        fi.reportResults(QVector<double>(n, 0));
    };
    QCOMPARE(Utils::runAsync<double>(fun, 2).results(),
             QList<double>({0, 0}));

    // operator()
    QCOMPARE(Utils::runAsync<double>(Callable(), 3).results(),
             QList<double>({0, 0, 0}));
    const Callable c{};
    QCOMPARE(Utils::runAsync<double>(c, 2).results(),
             QList<double>({0, 0}));

    // static member functions
    QCOMPARE(Utils::runAsync<double>(&MyObject::staticMember0).results(),
             QList<double>({0, 2, 1}));
    QCOMPARE(Utils::runAsync<double>(&MyObject::staticMember1, 2).results(),
             QList<double>({0, 0}));

    // member functions
    const MyObject obj{};
    QCOMPARE(Utils::runAsync<double>(&MyObject::member0, &obj).results(),
             QList<double>({0, 2, 1}));
    QCOMPARE(Utils::runAsync<double>(&MyObject::member1, &obj, 4).results(),
             QList<double>({0, 0, 0, 0}));
    QCOMPARE(Utils::runAsync<QString>(&MyObject::memberString1, &obj, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync<QString>(&MyObject::memberString1, &obj, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync<QString>(&MyObject::memberString1, &obj, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync<QString>(&MyObject::memberString1, &obj, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));
    QCOMPARE(Utils::runAsync<QString>(&MyObject::memberString2, &obj, s).results(),
             QList<QString>({s}));
    QCOMPARE(Utils::runAsync<QString>(&MyObject::memberString2, &obj, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(Utils::runAsync<QString>(&MyObject::memberString2, &obj, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(Utils::runAsync<QString>(&MyObject::memberString2, &obj, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));
}

QTEST_MAIN(tst_RunExtensions)

#include "tst_runextensions.moc"
