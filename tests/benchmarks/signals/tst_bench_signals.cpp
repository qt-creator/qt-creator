/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest>

#include <functional>
#include <vector>


/*
Config: Using QtTest library 5.10.0, Qt 5.10.0 (x86_64-little_endian-lp64 shared (dynamic)
    debug build; by GCC 7.2.1 20171205)

PASS   : BenchmarkSignals::qtMemfun()
     0.000068 msecs per iteration (total: 72, iterations: 1048576)
PASS   : BenchmarkSignals::qtLambda()
     0.000066 msecs per iteration (total: 70, iterations: 1048576)
PASS   : BenchmarkSignals::manualBind()
     0.00011 msecs per iteration (total: 60, iterations: 524288)
PASS   : BenchmarkSignals::manualLambda()
     0.000043 msecs per iteration (total: 91, iterations: 2097152)


CONFIG += release

********* Start testing of BenchmarkSignals *********
PASS   : BenchmarkSignals::qtMemfun()
     0.000061 msecs per iteration (total: 64, iterations: 1048576)
PASS   : BenchmarkSignals::qtLambda()
     0.000058 msecs per iteration (total: 61, iterations: 1048576)

 // Not completely surprising, as all the top call can be inlined

PASS   : BenchmarkSignals::manualBind()
     0.0000078 msecs per iteration (total: 66, iterations: 8388608)
PASS   : BenchmarkSignals::manualLambda()
     0.00000596 msecs per iteration (total: 100, iterations: 16777216)
PASS   : BenchmarkSignals::cleanupTestCase()


The latter in instructions:

PASS   : BenchmarkSignals::qtMemfun()
     445 instruction reads per iteration (total: 445, iterations: 1)
PASS   : BenchmarkSignals::qtLambda()
     440 instruction reads per iteration (total: 440, iterations: 1)
PASS   : BenchmarkSignals::manualBind()
     113 instruction reads per iteration (total: 113, iterations: 1)
PASS   : BenchmarkSignals::manualLambda()
     107 instruction reads per iteration (total: 107, iterations: 1)

*/


template <typename Type>
class NaiveSignal
{
public:
    using Callable = std::function<Type>;

    void connect(const Callable &callable) { m_callables.push_back(callable); }

    template <typename ...Args>
    void operator()(Args ...args) const
    {
        for (const Callable &callable : m_callables)
            callable(args...);
   }

private:
    std::vector<Callable> m_callables;
};


class BenchmarkSignals : public QObject
{
    Q_OBJECT

public:
    BenchmarkSignals() {}

private slots:
    void qtMemfun();
    void qtLambda();
    void manualBind();
    void manualLambda();
};

class Tester : public QObject
{
    Q_OBJECT

public:
    NaiveSignal<void(int)> naiveSignal;

signals:
    void qtSignal(int);

public:
    void doit(int i) { sum += i; }
    int sum = 0;
};

void BenchmarkSignals::qtMemfun()
{
    Tester tester;
    connect(&tester, &Tester::qtSignal, &tester, &Tester::doit);
    QBENCHMARK {
        tester.qtSignal(1);
    }
}

void BenchmarkSignals::qtLambda()
{
    Tester tester;
    connect(&tester, &Tester::qtSignal, [&](int x) { tester.sum += x; });
    QBENCHMARK {
        tester.qtSignal(1);
    }
}

void BenchmarkSignals::manualBind()
{
    Tester tester;
    tester.naiveSignal.connect(std::bind(&Tester::doit, &tester, std::placeholders::_1));
    QBENCHMARK {
        tester.naiveSignal(1);
    }
}

void BenchmarkSignals::manualLambda()
{
    Tester tester;
    tester.naiveSignal.connect([&](int x) { tester.sum += x; });
    QBENCHMARK {
        tester.naiveSignal(1);
    }
}

QTEST_MAIN(BenchmarkSignals)

#include "tst_bench_signals.moc"

