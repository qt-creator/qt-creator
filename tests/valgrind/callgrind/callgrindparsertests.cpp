/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "callgrindparsertests.h"

#include <valgrind/callgrind/callgrindparser.h>
#include <valgrind/callgrind/callgrindfunctioncall.h>
#include <valgrind/callgrind/callgrindfunction.h>
#include <valgrind/callgrind/callgrindfunctioncycle.h>
#include <valgrind/callgrind/callgrindcostitem.h>
#include <valgrind/callgrind/callgrindparsedata.h>

#include <QDebug>
#include <QFile>
#include <QString>
#include <QTest>

using namespace Valgrind;
using namespace Valgrind::Callgrind;

namespace {

static QString dataFile(const char *file)
{
    return QLatin1String(PARSERTESTS_DATA_DIR) + QLatin1String("/") + QLatin1String(file);
}

void testCostItem(const CostItem *item, quint64 expectedPosition, quint64 expectedCost)
{
    QCOMPARE(item->cost(0), expectedCost);
    QCOMPARE(item->position(0), expectedPosition);
}

void testSimpleCostItem(const CostItem *item, quint64 expectedPosition, quint64 expectedCost)
{
    QVERIFY(item->differingFileId() == -1);
    QVERIFY(!item->call());
    QCOMPARE(item->differingFileId(), qint64(-1));

    testCostItem(item, expectedPosition, expectedCost);
}

void testFunctionCall(const FunctionCall *call, const Function *caller, const Function *callee,
                      quint64 expectedCost, quint64 expectedCalls, quint64 expectedDestination)
{
    QCOMPARE(call->callee(), callee);
    QCOMPARE(call->caller(), caller);
    QCOMPARE(call->cost(0), expectedCost);
    QCOMPARE(call->calls(), expectedCalls);
    QCOMPARE(call->destination(0), expectedDestination);
}

void testCallCostItem(const CostItem *item, const Function *caller, const Function *callee,
                      quint64 expectedCalls, quint64 expectedDestination, quint64 expectedPosition,
                      quint64 expectedCost)
{
    QCOMPARE(item->differingFileId(), qint64(-1));
    testCostItem(item, expectedPosition, expectedCost);

    QVERIFY(item->call());
    testFunctionCall(item->call(), caller, callee, expectedCost, expectedCalls, expectedDestination);
}

void testDifferringFileCostItem(const CostItem *item, const QString &differingFile, quint64 expectedPosition, quint64 expectedCost)
{
    QVERIFY(!item->call());
    QCOMPARE(item->differingFile(), differingFile);

    testCostItem(item, expectedPosition, expectedCost);
}

}

void CallgrindParserTests::initTestCase()
{
}

void CallgrindParserTests::cleanup()
{
}

ParseData* parseDataFile(const QString &dataFile)
{
    QFile file(dataFile);
    Q_ASSERT(file.exists());
    file.open(QIODevice::ReadOnly);

    Parser p;
    p.parse(&file);

    return p.takeData();
}

void CallgrindParserTests::testHeaderData()
{
    QScopedPointer<const ParseData> data(parseDataFile(dataFile("simpleFunction.out")));

    QCOMPARE(data->command(), QLatin1String("ls"));
    QCOMPARE(data->creator(), QLatin1String("callgrind-3.6.0.SVN-Debian"));
    QCOMPARE(data->pid(), quint64(2992));
    QCOMPARE(data->version(), 1);
    QCOMPARE(data->part(), 1u);
    QCOMPARE(data->descriptions(), QStringList() << "I1 cache:" << "D1 cache:" << "L2 cache:"
                                                 << "Timerange: Basic block 0 - 300089"
                                                 << "Trigger: Program termination");

    QCOMPARE(data->positions(), QStringList() << "line");
    QCOMPARE(data->lineNumberPositionIndex(), 0);
    QCOMPARE(data->events(), QStringList() << "Ir");
    QCOMPARE(data->totalCost(0), quint64(1434186));
}

void CallgrindParserTests::testSimpleFunction()
{
    QScopedPointer<const ParseData> data(parseDataFile(dataFile("simpleFunction.out")));

    QCOMPARE(data->functions().size(), 4);

    {
    const Function *func = data->functions().at(0);
    QCOMPARE(func->file(), QLatin1String("/my/file.cpp"));
    QCOMPARE(func->object(), QLatin1String("/my/object"));
    QCOMPARE(func->name(), QLatin1String("myFunction"));

    QVERIFY(func->outgoingCalls().isEmpty());
    QCOMPARE(func->called(), quint64(0));
    QVERIFY(func->incomingCalls().isEmpty());
    QCOMPARE(func->costItems().size(), 7);
    testSimpleCostItem(func->costItems().at(0), 1, 1);
    testSimpleCostItem(func->costItems().at(1), 3, 1);
    testSimpleCostItem(func->costItems().at(2), 3, 3);
    testSimpleCostItem(func->costItems().at(3), 1, 1);
    testSimpleCostItem(func->costItems().at(4), 5, 4);
    testDifferringFileCostItem(func->costItems().at(5), QLatin1String("/my/file3.h"), 1, 5);
    testSimpleCostItem(func->costItems().at(6), 7, 5);
    QCOMPARE(func->selfCost(0), quint64(20));
    QCOMPARE(func->inclusiveCost(0), quint64(20));
    }

    {
    const Function *func = data->functions().at(1);
    QCOMPARE(func->file(), QLatin1String("/my/file.cpp"));
    QCOMPARE(func->object(), QLatin1String("/my/object"));
    QCOMPARE(func->name(), QLatin1String("myFunction2"));

    QVERIFY(func->incomingCalls().isEmpty());
    QCOMPARE(func->called(), quint64(0));
    QVERIFY(func->outgoingCalls().isEmpty());
    QCOMPARE(func->costItems().size(), 1);
    QCOMPARE(func->selfCost(0), quint64(1));
    QCOMPARE(func->inclusiveCost(0), func->selfCost(0));
    }

    {
    const Function *func = data->functions().at(2);
    QCOMPARE(func->file(), QLatin1String("/my/file2.cpp"));
    QCOMPARE(func->object(), QLatin1String("/my/object"));
    QCOMPARE(func->name(), QLatin1String("myFunction4"));

    QVERIFY(func->incomingCalls().isEmpty());
    QCOMPARE(func->called(), quint64(0));
    QVERIFY(func->outgoingCalls().isEmpty());
    QCOMPARE(func->costItems().size(), 1);
    QCOMPARE(func->selfCost(0), quint64(1));
    QCOMPARE(func->inclusiveCost(0), func->selfCost(0));
    }

    {
    const Function *func = data->functions().at(3);
    QCOMPARE(func->file(), QLatin1String("/my/file.cpp"));
    QCOMPARE(func->object(), QLatin1String("/my/object2"));
    QCOMPARE(func->name(), QLatin1String("myFunction3"));

    QVERIFY(func->incomingCalls().isEmpty());
    QCOMPARE(func->called(), quint64(0));
    QVERIFY(func->outgoingCalls().isEmpty());
    QCOMPARE(func->costItems().size(), 1);
    QCOMPARE(func->selfCost(0), quint64(1));
    }
}

void CallgrindParserTests::testCallee()
{
    QScopedPointer<const ParseData> data(parseDataFile(dataFile("calleeFunctions.out")));

    QCOMPARE(data->functions().size(), 3);

    // basic function data testing
    const Function *main = data->functions().at(0);
    QCOMPARE(main->file(), QLatin1String("file1.c"));
    QCOMPARE(main->object(), QLatin1String(""));
    QCOMPARE(main->name(), QLatin1String("main"));

    QVERIFY(main->incomingCalls().isEmpty());
    QCOMPARE(main->called(), quint64(0));
    QCOMPARE(main->outgoingCalls().size(), 2);
    QCOMPARE(main->costItems().size(), 5);
    testSimpleCostItem(main->costItems().at(0), 16, 20);
    testSimpleCostItem(main->costItems().at(3), 17, 10);
    QCOMPARE(main->selfCost(0), quint64(30));
    QCOMPARE(main->inclusiveCost(0), quint64(1230));

    const Function *func1 = data->functions().at(1);
    QCOMPARE(func1->file(), QLatin1String("file1.c"));
    QCOMPARE(func1->object(), QLatin1String(""));
    QCOMPARE(func1->name(), QLatin1String("func1"));

    QCOMPARE(func1->incomingCalls().size(), 1);
    QCOMPARE(func1->called(), quint64(1));
    QCOMPARE(func1->outgoingCalls().size(), 1);
    QCOMPARE(func1->costItems().size(), 2);
    testSimpleCostItem(func1->costItems().at(0), 51, 100);
    QCOMPARE(func1->selfCost(0), quint64(100));
    QCOMPARE(func1->inclusiveCost(0), quint64(400));

    const Function *func2 = data->functions().at(2);
    QCOMPARE(func2->file(), QLatin1String("file2.c"));
    QCOMPARE(func2->object(), QLatin1String(""));
    QCOMPARE(func2->name(), QLatin1String("func2"));

    QCOMPARE(func2->incomingCalls().size(), 2);
    QCOMPARE(func2->called(), quint64(8));
    QVERIFY(func2->outgoingCalls().isEmpty());
    QCOMPARE(func2->costItems().size(), 1);
    testSimpleCostItem(func2->costItems().at(0), 20, 700);
    QCOMPARE(func2->selfCost(0), quint64(700));
    QCOMPARE(func2->inclusiveCost(0), quint64(700));

    // now test callees
    testCallCostItem(main->costItems().at(1), main, func1, 1, 50, 16, 400);
    testCallCostItem(main->costItems().at(2), main, func2, 3, 20, 16, 400);

    testCallCostItem(func1->costItems().at(1), func1, func2, 2, 20, 51, 300);

    // order is undefined
    if (func2->incomingCalls().first()->caller() == func1) {
        testFunctionCall(func2->incomingCalls().first(), func1, func2, 300, 2, 20);
        testFunctionCall(func2->incomingCalls().last(), main, func2, 800, 6, 20);
    } else {
        testFunctionCall(func2->incomingCalls().last(), func1, func2, 300, 2, 20);
        testFunctionCall(func2->incomingCalls().first(), main, func2, 800, 6, 20);
    }

    // order is undefined
    if (main->outgoingCalls().first()->callee() == func2) {
        testFunctionCall(main->outgoingCalls().first(), main, func2, 800, 6, 20);
        testFunctionCall(main->outgoingCalls().last(), main, func1, 400, 1, 50);
    } else {
        testFunctionCall(main->outgoingCalls().last(), main, func2, 800, 6, 20);
        testFunctionCall(main->outgoingCalls().first(), main, func1, 400, 1, 50);
    }

    testFunctionCall(func1->outgoingCalls().first(), func1, func2, 300, 2, 20);
    testFunctionCall(func1->incomingCalls().first(), main, func1, 400, 1, 50);
}

void CallgrindParserTests::testInlinedCalls()
{
    QScopedPointer<const ParseData> data(parseDataFile(dataFile("inlinedFunctions.out")));
    QCOMPARE(data->functions().size(), 3);

    const Function *main = data->functions().first();
    QCOMPARE(main->name(), QLatin1String("main"));
    QCOMPARE(main->file(), QLatin1String("file1.c"));
    QCOMPARE(main->selfCost(0), quint64(4));
    QCOMPARE(main->inclusiveCost(0), quint64(804));

    const Function *inlined = data->functions().at(1);
    QCOMPARE(inlined->name(), QLatin1String("Something::Inlined()"));
    QCOMPARE(inlined->file(), QLatin1String("file.h"));

    const Function *func1 = data->functions().last();
    QCOMPARE(func1->name(), QLatin1String("func1"));
    QCOMPARE(func1->file(), QLatin1String("file3.h"));

    QCOMPARE(main->outgoingCalls().size(), 2);
    QCOMPARE(main->costItems().at(2)->call()->callee(), inlined);
    QCOMPARE(main->costItems().at(3)->call()->callee(), func1);

    testSimpleCostItem(main->costItems().last(), 1, 2);
}

void CallgrindParserTests::testMultiCost()
{
    QScopedPointer<const ParseData> data(parseDataFile(dataFile("multiCost.out")));
    QCOMPARE(data->functions().size(), 2);

    QCOMPARE(data->positions(), QStringList() << "line");
    QCOMPARE(data->events(), QStringList() << "Ir" << "Time");

    QCOMPARE(data->totalCost(0), quint64(4));
    QCOMPARE(data->totalCost(1), quint64(400));

    const Function *main = data->functions().at(0);
    QCOMPARE(main->costItems().first()->costs(), QVector<quint64>() << 1 << 100);
    QCOMPARE(main->costItems().first()->positions(), QVector<quint64>() << 1);

    QVERIFY(main->costItems().last()->call());
    QCOMPARE(main->costItems().last()->call()->destinations(), QVector<quint64>() << 1);
}

void CallgrindParserTests::testMultiPos()
{
    QScopedPointer<const ParseData> data(parseDataFile(dataFile("multiPos.out")));
    QCOMPARE(data->functions().size(), 2);

    QCOMPARE(data->positions(), QStringList() << "line" << "memAddr");
    QCOMPARE(data->events(), QStringList() << "Ir");

    QCOMPARE(data->totalCost(0), quint64(4));

    const Function *main = data->functions().at(0);
    QCOMPARE(main->costItems().first()->costs(), QVector<quint64>() << 1);
    QCOMPARE(main->costItems().first()->positions(), QVector<quint64>() << 1 << 0x01);

    QVERIFY(main->costItems().last()->call());
    QCOMPARE(main->costItems().last()->call()->destinations(), QVector<quint64>() << 1 << 0x04);
}

void CallgrindParserTests::testMultiPosAndCost()
{
    QScopedPointer<const ParseData> data(parseDataFile(dataFile("multiCostAndPos.out")));
    QCOMPARE(data->functions().size(), 2);

    QCOMPARE(data->positions(), QStringList() << "line" << "memAddr");
    QCOMPARE(data->events(), QStringList() << "Ir" << "Time");

    QCOMPARE(data->totalCost(0), quint64(4));
    QCOMPARE(data->totalCost(1), quint64(400));

    const Function *main = data->functions().at(0);
    QCOMPARE(main->costItems().first()->costs(), QVector<quint64>() << 1 << 100);
    QCOMPARE(main->costItems().first()->positions(), QVector<quint64>() << 1 << 0x01);

    QVERIFY(main->costItems().last()->call());
    QCOMPARE(main->costItems().last()->call()->destinations(), QVector<quint64>() << 1 << 0x04);
}

const Function *findFunction(const QString &needle, const QVector<const Function *> &haystack)
{
    foreach (const Function *function, haystack) {
        if (function->name() == needle) {
            return function;
        }
    }
    return 0;
}

void CallgrindParserTests::testCycle()
{
    QScopedPointer<const ParseData> data(parseDataFile(dataFile("cycle.out")));
    QCOMPARE(data->functions().size(), 4);

    const Function *main = data->functions().at(0);
    QCOMPARE(main->name(), QLatin1String("main"));
    QCOMPARE(main->inclusiveCost(0), quint64(80));
    QCOMPARE(main->selfCost(0), quint64(30));
    const Function *a = data->functions().at(1);
    QCOMPARE(a->name(), QLatin1String("A()"));
    QCOMPARE(a->inclusiveCost(0), quint64(50));
    QCOMPARE(a->selfCost(0), quint64(10));
    const Function *c = data->functions().at(2);
    QCOMPARE(c->name(), QLatin1String("C(bool)"));
    QCOMPARE(c->inclusiveCost(0), quint64(30));
    QCOMPARE(c->selfCost(0), quint64(20));
    const Function *b = data->functions().at(3);
    QCOMPARE(b->name(), QLatin1String("B(bool)"));
    QCOMPARE(b->inclusiveCost(0), quint64(30));
    QCOMPARE(b->selfCost(0), quint64(20));

    QCOMPARE(data->functions(true).size(), 3);
    QCOMPARE(findFunction(QLatin1String("main"), data->functions(true)), main);
    QCOMPARE(findFunction(QLatin1String("A()"), data->functions(true)), a);
    const FunctionCycle *cycle = dynamic_cast<const FunctionCycle*>(findFunction(QLatin1String("cycle 1"), data->functions(true)));
    QVERIFY(cycle);
    QCOMPARE(cycle->called(), quint64(2));
    QCOMPARE(cycle->inclusiveCost(0), quint64(40));
    QCOMPARE(cycle->selfCost(0), quint64(40));
}

void CallgrindParserTests::testRecursiveCycle()
{
    QScopedPointer<const ParseData> data(parseDataFile(dataFile("recursiveCycle.out")));
    QCOMPARE(data->functions().size(), 5);

    const Function *main = findFunction(QLatin1String("main"), data->functions());
    QVERIFY(main);
    QCOMPARE(main->inclusiveCost(0), quint64(70701765 + 3 + 4));
    QCOMPARE(data->totalCost(0), main->inclusiveCost(0));
    QCOMPARE(main->selfCost(0), quint64(3 + 4));
    const Function *a1 = findFunction(QLatin1String("A(int)"), data->functions());
    QVERIFY(a1);
    QCOMPARE(a1->inclusiveCost(0), quint64(700017 + 70001746 + 2));
    QCOMPARE(a1->selfCost(0), quint64(700017 + 2));
    const Function *a2 = findFunction(QLatin1String("A(int)'2"), data->functions());
    QVERIFY(a2);
    QCOMPARE(a2->inclusiveCost(0), quint64(35000846 + 1715042679 + 100));
    QCOMPARE(a2->selfCost(0), quint64(35000846 + 100));
    const Function *b1 = findFunction(QLatin1String("B(int)"), data->functions());
    QVERIFY(b1);
    QCOMPARE(b1->inclusiveCost(0), quint64(700014 + 69301730 + 2));
    QCOMPARE(b1->selfCost(0), quint64(700014 + 2));
    const Function *b2 = findFunction(QLatin1String("B(int)'2"), data->functions());
    QVERIFY(b2);
    QCOMPARE(b2->inclusiveCost(0), quint64(34300686 + 1680741895 + 98));
    QCOMPARE(b2->selfCost(0), quint64(34300686 + 98));

    { // cycle detection
        QCOMPARE(data->functions(true).size(), 4);
        QCOMPARE(findFunction(QLatin1String("main"), data->functions(true)), main);
        QCOMPARE(findFunction(QLatin1String("A(int)"), data->functions(true)), a1);
        QCOMPARE(findFunction(QLatin1String("B(int)"), data->functions(true)), b1);
        const FunctionCycle *cycle = dynamic_cast<const FunctionCycle*>(findFunction(QLatin1String("cycle 1"), data->functions(true)));
        QVERIFY(cycle);
        QCOMPARE(cycle->called(), quint64(1));
        const quint64 restCost = data->totalCost(0) - main->selfCost(0) - a1->selfCost(0) - b1->selfCost(0);
        QCOMPARE(cycle->inclusiveCost(0), restCost);
        QCOMPARE(cycle->selfCost(0), restCost);
    }
}

void CallgrindParserTests::testRecursion()
{
    QScopedPointer<const ParseData> data(parseDataFile(dataFile("recursion.out")));
    QCOMPARE(data->functions().size(), 3);
    QCOMPARE(data->totalCost(0), quint64(35700972));

    const Function *main = findFunction(QLatin1String("main"), data->functions());
    QVERIFY(main);
    QCOMPARE(main->inclusiveCost(0), quint64(4 + 35700965 + 3));
    QCOMPARE(main->selfCost(0), quint64(4 + 3));

    const Function *a1 = findFunction(QLatin1String("A(int)"), data->functions());
    QVERIFY(a1);
    QCOMPARE(a1->inclusiveCost(0), quint64(700010 + 35000946 + 2));
    QCOMPARE(a1->selfCost(0), quint64(700010 + 2));

    const Function *a2 = findFunction(QLatin1String("A(int)'2"), data->functions());
    QVERIFY(a2);
    // inclusive cost must be the same as call-cost from a1
    QCOMPARE(a2->inclusiveCost(0), quint64(35000946));
    QCOMPARE(a2->selfCost(0), quint64(35000846));

}

QTEST_APPLESS_MAIN(CallgrindParserTests)
