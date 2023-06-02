// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>
#include <tracing/timelinemodelaggregator.h>

using namespace Timeline;

class tst_TimelineModelAggregator : public QObject
{
    Q_OBJECT

private slots:
    void height();
    void addRemoveModel();
    void prevNext();
};

class HeightTestModel : public TimelineModel {
public:
    HeightTestModel(TimelineModelAggregator *parent) : TimelineModel(parent)
    {
        insert(0, 1, 1);
    }
};

void tst_TimelineModelAggregator::height()
{
    TimelineModelAggregator aggregator;
    QCOMPARE(aggregator.height(), 0);

    QSignalSpy heightSpy(&aggregator, &TimelineModelAggregator::heightChanged);
    TimelineModel *model = new TimelineModel(&aggregator);
    aggregator.addModel(model);
    QCOMPARE(aggregator.height(), 0);
    QCOMPARE(heightSpy.count(), 0);
    aggregator.addModel(new HeightTestModel(&aggregator));
    QVERIFY(aggregator.height() > 0);
    QCOMPARE(heightSpy.count(), 1);
    aggregator.clear();
    QCOMPARE(aggregator.height(), 0);
    QCOMPARE(heightSpy.count(), 2);
}

void tst_TimelineModelAggregator::addRemoveModel()
{
    TimelineNotesModel notes;
    TimelineModelAggregator aggregator;
    aggregator.setNotes(&notes);
    QSignalSpy spy(&aggregator, &TimelineModelAggregator::modelsChanged);

    QCOMPARE(aggregator.notes(), &notes);

    TimelineModel *model1 = new TimelineModel(&aggregator);
    TimelineModel *model2 = new TimelineModel(&aggregator);
    aggregator.addModel(model1);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(aggregator.modelCount(), 1);
    QCOMPARE(aggregator.model(0), model1);
    QCOMPARE(aggregator.models().count(), 1);

    aggregator.addModel(model2);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(aggregator.modelCount(), 2);
    QCOMPARE(aggregator.model(1), model2);
    QCOMPARE(aggregator.models().count(), 2);

    QCOMPARE(aggregator.modelIndexById(model1->modelId()), 0);
    QCOMPARE(aggregator.modelIndexById(model2->modelId()), 1);
    QCOMPARE(aggregator.modelIndexById(aggregator.generateModelId()), -1);

    QCOMPARE(aggregator.modelOffset(0), 0);
    QCOMPARE(aggregator.modelOffset(1), 0);

    aggregator.clear();
    QCOMPARE(spy.count(), 3);
    QCOMPARE(aggregator.modelCount(), 0);
}

class PrevNextTestModel : public TimelineModel
{
public:
    PrevNextTestModel(TimelineModelAggregator *parent) : TimelineModel(parent)
    {
        for (int i = 0; i < 20; ++i)
            insert(i + modelId(), i * modelId(), modelId());
    }
};

void tst_TimelineModelAggregator::prevNext()
{
    TimelineModelAggregator aggregator;
    aggregator.generateModelId(); // start modelIds at 1
    aggregator.addModel(new PrevNextTestModel(&aggregator));
    aggregator.addModel(new PrevNextTestModel(&aggregator));
    aggregator.addModel(new PrevNextTestModel(&aggregator));

    // Add an empty model to trigger the special code paths that skip it
    aggregator.addModel(new TimelineModel(&aggregator));
    QLatin1String item("item");
    QLatin1String model("model");
    QVariantMap result;

    {
        int indexes[] = {-1, -1, -1};
        int lastModel = -1;
        int lastIndex = -1;
        for (int i = 0; i < 60; ++i) {
            result = aggregator.nextItem(lastModel, lastIndex, -1);
            int nextModel = result.value(model).toInt();
            int nextIndex = result.value(item).toInt();
            QVERIFY(nextModel < 3);
            QVERIFY(nextModel >= 0);
            QCOMPARE(nextIndex, ++indexes[nextModel]);
            lastModel = nextModel;
            lastIndex = nextIndex;
        }

        result = aggregator.nextItem(lastModel, lastIndex, -1);
        QCOMPARE(result.value(model).toInt(), 0);
        QCOMPARE(result.value(item).toInt(), 0);
    }

    {
        int indexes[] = {20, 20, 20};
        int lastModel = -1;
        int lastIndex = -1;
        for (int i = 0; i < 60; ++i) {
            result = aggregator.prevItem(lastModel, lastIndex, -1);
            int prevModel = result.value(model).toInt();
            int prevIndex = result.value(item).toInt();
            QVERIFY(prevModel < 3);
            QVERIFY(prevModel >= 0);
            QCOMPARE(prevIndex, --indexes[prevModel]);
            lastModel = prevModel;
            lastIndex = prevIndex;
        }

        result = aggregator.prevItem(lastModel, lastIndex, -1);
        QCOMPARE(result.value(model).toInt(), 2);
        QCOMPARE(result.value(item).toInt(), 19);
    }
}

QTEST_GUILESS_MAIN(tst_TimelineModelAggregator)

#include "tst_timelinemodelaggregator.moc"
