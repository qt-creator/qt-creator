// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>

#include <QTest>

using namespace Timeline;

class DummyModel : public TimelineModel
{
public:
    DummyModel(TimelineModelAggregator *parent) : TimelineModel(parent) {}
};

class tst_TimelineModelState : public QObject
{
    Q_OBJECT

private slots:
    void emptyModel();
    void defaultRowHeight();
    void rowCountDefault();
};

void tst_TimelineModelState::emptyModel()
{
    TimelineModelAggregator aggregator;
    DummyModel model(&aggregator);
    QCOMPARE(model.count(), 0);
    QVERIFY(model.isEmpty());
    QCOMPARE(model.firstIndex(0), -1);
    QCOMPARE(model.lastIndex(0), -1);
}

void tst_TimelineModelState::defaultRowHeight()
{
    QVERIFY(TimelineModel::defaultRowHeight() > 0);
}

void tst_TimelineModelState::rowCountDefault()
{
    TimelineModelAggregator aggregator;
    DummyModel model(&aggregator);
    QCOMPARE(model.collapsedRowCount(), 1);
    QCOMPARE(model.expandedRowCount(), 1);
}

QTEST_GUILESS_MAIN(tst_TimelineModelState)

#include "tst_timelinemodelstate.moc"
