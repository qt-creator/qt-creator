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

#include <QtTest>
#include <timeline/timelinemodelaggregator.h>

class tst_TimelineModelAggregator : public QObject
{
    Q_OBJECT

private slots:
    void height();
    void addRemoveModel();
    void prevNext();
};

class HeightTestModel : public Timeline::TimelineModel {
public:
    HeightTestModel() : TimelineModel(2, QString())
    {
        insert(0, 1, 1);
    }
};

void tst_TimelineModelAggregator::height()
{
    Timeline::TimelineModelAggregator aggregator(0);
    QCOMPARE(aggregator.height(), 0);

    QSignalSpy heightSpy(&aggregator, SIGNAL(heightChanged()));
    Timeline::TimelineModel *model = new Timeline::TimelineModel(25, QString());
    aggregator.addModel(model);
    QCOMPARE(aggregator.height(), 0);
    QCOMPARE(heightSpy.count(), 0);
    aggregator.addModel(new HeightTestModel);
    QVERIFY(aggregator.height() > 0);
    QCOMPARE(heightSpy.count(), 1);
    aggregator.clear();
    QCOMPARE(aggregator.height(), 0);
    QCOMPARE(heightSpy.count(), 2);
}

void tst_TimelineModelAggregator::addRemoveModel()
{
    Timeline::TimelineNotesModel notes;
    Timeline::TimelineModelAggregator aggregator(&notes);
    QSignalSpy spy(&aggregator, SIGNAL(modelsChanged()));

    QCOMPARE(aggregator.notes(), &notes);

    Timeline::TimelineModel *model1 = new Timeline::TimelineModel(25, QString());
    Timeline::TimelineModel *model2 = new Timeline::TimelineModel(26, QString());
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

    QCOMPARE(aggregator.modelIndexById(25), 0);
    QCOMPARE(aggregator.modelIndexById(26), 1);
    QCOMPARE(aggregator.modelIndexById(27), -1);

    QCOMPARE(aggregator.modelOffset(0), 0);
    QCOMPARE(aggregator.modelOffset(1), 0);

    aggregator.clear();
    QCOMPARE(spy.count(), 3);
    QCOMPARE(aggregator.modelCount(), 0);
}

class PrevNextTestModel : public Timeline::TimelineModel
{
public:
    PrevNextTestModel(int x) : TimelineModel(x, QString())
    {
        for (int i = 0; i < 20; ++i)
            insert(i + x, i * x, x);
    }
};

void tst_TimelineModelAggregator::prevNext()
{
    Timeline::TimelineModelAggregator aggregator(0);
    aggregator.addModel(new PrevNextTestModel(1));
    aggregator.addModel(new PrevNextTestModel(2));
    aggregator.addModel(new PrevNextTestModel(3));

    // Add an empty model to trigger the special code paths that skip it
    aggregator.addModel(new Timeline::TimelineModel(4, QString()));
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

QTEST_MAIN(tst_TimelineModelAggregator)

#include "tst_timelinemodelaggregator.moc"
