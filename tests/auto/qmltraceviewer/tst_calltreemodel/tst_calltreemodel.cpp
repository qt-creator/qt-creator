// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "calltreemodel.h"

#include <QtTest>
#include <QAbstractItemModelTester>

using namespace QmlProfiler::Internal;

// labels: 0=start 1=main 2=work 3=idle 4=helper
static SampleTraceData makeTestData()
{
    SampleTraceData data;
    data.pid = 1;
    data.labels = {"start", "main", "work", "idle", "helper"};
    data.samples = {
        {0, 10, true, {0, 1, 2}},    // start>main>work
        {0, 11, true, {0, 1, 4}},    // start>main>helper
        {100, 10, true, {0, 1, 2}},  // start>main>work
        {100, 11, false, {0, 3}},    // idle thread: must be ignored
        {200, 10, true, {0, 3}},     // start>idle
    };
    return data;
}

class tst_CallTreeModel : public QObject
{
    Q_OBJECT

private slots:
    void mergesPrefixesAndCountsOnCpuOnly()
    {
        SampleTraceData data = makeTestData();
        CallTreeModel model;
        model.setTraceData(&data);

        QCOMPARE(model.totalWeight(), quint64(4)); // the !running sample is excluded

        // One root: "start" with weight 4.
        QCOMPARE(model.rowCount(), 1);
        const QModelIndex start = model.index(0, CallTreeModel::SymbolColumn);
        QCOMPARE(model.data(start, Qt::DisplayRole).toString(), QString("start"));
        const CallTreeModel::Node *startNode = model.node(start);
        QVERIFY(startNode);
        QCOMPARE(startNode->weight, quint64(4));
        QCOMPARE(startNode->self, quint64(0));

        // start's children sorted heaviest-first: main (3), idle (1).
        QCOMPARE(model.rowCount(model.index(0, 0)), 2);
        const CallTreeModel::Node *mainNode = model.node(model.index(0, 0, model.index(0, 0)));
        QCOMPARE(model.symbol(mainNode), QString("main"));
        QCOMPARE(mainNode->weight, quint64(3));
        const CallTreeModel::Node *idleNode = model.node(model.index(1, 0, model.index(0, 0)));
        QCOMPARE(model.symbol(idleNode), QString("idle"));
        QCOMPARE(idleNode->weight, quint64(1));
        QCOMPARE(idleNode->self, quint64(1));

        // main's children: work (2, self 2), helper (1, self 1).
        QCOMPARE(mainNode->childCount(), 2);
        QCOMPARE(model.symbol(mainNode->child(0)), QString("work"));
        QCOMPARE(mainNode->child(0)->weight, quint64(2));
        QCOMPARE(mainNode->child(0)->self, quint64(2));
        QCOMPARE(model.symbol(mainNode->child(1)), QString("helper"));
        QCOMPARE(mainNode->child(1)->self, quint64(1));
    }

    void heaviestPathDescendsHeaviestChildren()
    {
        SampleTraceData data = makeTestData();
        CallTreeModel model;
        model.setTraceData(&data);

        const QList<const CallTreeModel::Node *> path = model.heaviestPath();
        QCOMPARE(path.size(), 3);
        QCOMPARE(model.symbol(path[0]), QString("start"));
        QCOMPARE(model.symbol(path[1]), QString("main"));
        QCOMPARE(model.symbol(path[2]), QString("work"));

        // Starting from a node includes that node.
        const QList<const CallTreeModel::Node *> fromMain = model.heaviestPath(path[1]);
        QCOMPARE(fromMain.size(), 2);
        QCOMPARE(model.symbol(fromMain[0]), QString("main"));
        QCOMPARE(model.symbol(fromMain[1]), QString("work"));
    }

    void timeRangeFiltersSamples()
    {
        SampleTraceData data = makeTestData();
        CallTreeModel model;
        model.setTraceData(&data);

        model.setTimeRange(0, 100);
        QCOMPARE(model.totalWeight(), quint64(3)); // t=200 sample excluded
        const CallTreeModel::Node *startNode = model.node(model.index(0, 0));
        QCOMPARE(startNode->weight, quint64(3));
        QCOMPARE(startNode->childCount(), 1); // "idle" path is gone

        model.setTimeRange(-1, -1); // back to the full recording
        QCOMPARE(model.totalWeight(), quint64(4));
    }

    void weightColumnShowsPercent()
    {
        SampleTraceData data = makeTestData();
        CallTreeModel model;
        model.setTraceData(&data);
        const QString weight
            = model.data(model.index(0, CallTreeModel::WeightColumn), Qt::DisplayRole).toString();
        QVERIFY2(weight.contains("4") && weight.contains("100.0"), qPrintable(weight));
    }

    void locationFromLabel()
    {
        SampleTraceData data;
        data.pid = 1;
        data.labels = {{"root", "/r.cpp", 5}, {"leaf"}}; // leaf has no source info
        data.samples = {{0, 10, true, {0, 1}}};
        CallTreeModel model;
        model.setTraceData(&data);

        const QModelIndex rootIdx = model.index(0, 0);
        const CallTreeModel::Node *root = model.node(rootIdx);
        QCOMPARE(model.symbol(root), QString("root"));
        QCOMPARE(model.location(root).file, QString("/r.cpp"));
        QCOMPARE(model.location(root).line, 5);

        const CallTreeModel::Node *leaf = model.node(model.index(0, 0, rootIdx));
        QCOMPARE(model.symbol(leaf), QString("leaf"));
        QVERIFY(model.location(leaf).file.isEmpty());
        QCOMPARE(model.location(leaf).line, 0);
        QVERIFY(model.location(nullptr).file.isEmpty()); // null-safe
    }

    void clearedModelIsEmpty()
    {
        SampleTraceData data = makeTestData();
        CallTreeModel model;
        model.setTraceData(&data);
        model.setTraceData(nullptr);
        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(model.totalWeight(), quint64(0));
        QVERIFY(model.heaviestPath().isEmpty());
    }

    void symbolIsTheTreeColumn()
    {
        SampleTraceData data = makeTestData();
        CallTreeModel model;
        model.setTraceData(&data);

        // The symbol carries the tree decoration, so it must stay column 0.
        QCOMPARE(int(CallTreeModel::SymbolColumn), 0);
        QCOMPARE(model.headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(),
                 QString("Symbol"));
        QCOMPARE(model.headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(),
                 QString("Weight"));
        QCOMPARE(model.headerData(2, Qt::Horizontal, Qt::DisplayRole).toString(),
                 QString("Self"));
    }

    void modelContractHolds()
    {
        SampleTraceData data = makeTestData();
        CallTreeModel model;
        model.setTraceData(&data);
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);

        // indexFor/node round-trip and parent consistency through the
        // QModelIndex API.
        const QModelIndex start = model.index(0, 0);
        const CallTreeModel::Node *startNode = model.node(start);
        QCOMPARE(model.indexFor(startNode), start);
        const QModelIndex main = model.index(0, 0, start);
        QCOMPARE(model.parent(main), start);
        QCOMPARE(model.node(model.indexFor(model.node(main))), model.node(main));

        // A rebuild invalidates outstanding indexes via the model reset.
        QPersistentModelIndex persistent(main);
        QVERIFY(persistent.isValid());
        model.setTimeRange(0, 100);
        QVERIFY(!persistent.isValid());
    }
};

QTEST_GUILESS_MAIN(tst_CallTreeModel)

#include "tst_calltreemodel.moc"
