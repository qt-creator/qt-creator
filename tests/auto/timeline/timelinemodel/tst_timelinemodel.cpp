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

#include <QtTest>
#include <QColor>
#include <timeline/timelinemodel_p.h>

static const int NumItems = 32;
static const qint64 ItemDuration = 1 << 19;
static const qint64 ItemSpacing = 1 << 20;

class DummyModel : public Timeline::TimelineModel
{
    Q_OBJECT
    friend class tst_TimelineModel;
public:
    DummyModel(int modelId);
    DummyModel(QString displayName = tr("dummy"), QObject *parent = 0);
    int expandedRow(int) const { return 2; }
    int collapsedRow(int) const { return 1; }

protected:
    void loadData();
};

class tst_TimelineModel : public QObject
{
    Q_OBJECT

public:
    const int DefaultRowHeight;
    tst_TimelineModel();

private slots:
    void privateModel();
    void isEmpty();
    void modelId();
    void rowHeight();
    void rowOffset();
    void height();
    void count();
    void times();
    void selectionId();
    void firstLast();
    void expand();
    void hide();
    void displayName();
    void defaultValues();
    void row();
    void colorByHue();
    void colorBySelectionId();
    void colorByFraction();
    void supportedRenderPasses();
    void insertStartEnd();
    void rowCount();
    void prevNext();
    void parentingOfEqualStarts();
};

DummyModel::DummyModel(int modelId) :
    Timeline::TimelineModel(modelId, 0)
{
}

DummyModel::DummyModel(QString displayName, QObject *parent) :
    TimelineModel(12, parent)
{
    setDisplayName(displayName);
}

void DummyModel::loadData()
{
    // TimelineModel is clever enough to sort the longer events before the short ones with the same
    // start time to avoid unnecessary complications when assigning parents. So, if you do e.g.
    // firstIndex(0) you get the first event insered in the third iteration of the loop.

    for (int i = 0; i < NumItems / 4; ++i) {
        insert((i / 3) * ItemSpacing, ItemDuration * (i + 1) + 2, 4);
        insert((i / 3) * ItemSpacing + 1, ItemDuration * (i + 1), 5);
        insert(i * ItemSpacing / 3 + 1, ItemDuration * (i + 1) + 3, 6);
        insert(i * ItemSpacing / 3 + 2, 1, 7);
    }
    computeNesting();
    setCollapsedRowCount(2);
    setExpandedRowCount(3);
}

tst_TimelineModel::tst_TimelineModel() :
    DefaultRowHeight(Timeline::TimelineModel::defaultRowHeight())
{
}

void tst_TimelineModel::privateModel()
{
    DummyModel dummy(15);
    QCOMPARE(dummy.modelId(), 15);
}

void tst_TimelineModel::isEmpty()
{
    DummyModel dummy;
    QVERIFY(dummy.isEmpty());
    dummy.loadData();
    QVERIFY(!dummy.isEmpty());
    dummy.clear();
    QVERIFY(dummy.isEmpty());
}

void tst_TimelineModel::modelId()
{
    DummyModel dummy;
    QCOMPARE(dummy.modelId(), 12);
}

void tst_TimelineModel::rowHeight()
{
    DummyModel dummy;
    QCOMPARE(dummy.rowHeight(0), DefaultRowHeight);
    QCOMPARE(dummy.collapsedRowHeight(0), DefaultRowHeight);
    QCOMPARE(dummy.expandedRowHeight(0), DefaultRowHeight);

    dummy.setExpandedRowHeight(0, 100);
    QCOMPARE(dummy.rowHeight(0), DefaultRowHeight);
    QCOMPARE(dummy.collapsedRowHeight(0), DefaultRowHeight);
    QCOMPARE(dummy.expandedRowHeight(0), 100);

    dummy.setExpanded(true);
    QCOMPARE(dummy.rowHeight(0), 100);

    // Cannot set smaller than default
    dummy.setExpandedRowHeight(0, DefaultRowHeight - 1);
    QCOMPARE(dummy.rowHeight(0), DefaultRowHeight);

    dummy.setExpandedRowHeight(0, 100);
    QCOMPARE(dummy.rowHeight(0), 100);

    dummy.loadData();
    dummy.setExpandedRowHeight(1, 50);
    QCOMPARE(dummy.rowHeight(0), 100);
    QCOMPARE(dummy.rowHeight(1), 50);

    // Row heights ignored while collapsed ...
    dummy.setExpanded(false);
    QCOMPARE(dummy.rowHeight(0), DefaultRowHeight);
    QCOMPARE(dummy.rowHeight(1), DefaultRowHeight);

    // ... but restored when re-expanding
    dummy.setExpanded(true);
    QCOMPARE(dummy.rowHeight(0), 100);
    QCOMPARE(dummy.rowHeight(1), 50);

    QSignalSpy expandedSpy(&dummy, SIGNAL(expandedRowHeightChanged(int,int)));
    dummy.clear();
    QCOMPARE(expandedSpy.count(), 1);
}

void tst_TimelineModel::rowOffset()
{
    DummyModel dummy;
    QCOMPARE(dummy.rowOffset(0), 0);

    dummy.loadData();
    dummy.setExpanded(true);
    QCOMPARE(dummy.rowOffset(0), 0);
    QCOMPARE(dummy.rowOffset(1), DefaultRowHeight);
    QCOMPARE(dummy.collapsedRowOffset(1), DefaultRowHeight);
    QCOMPARE(dummy.expandedRowOffset(1), DefaultRowHeight);

    dummy.setExpandedRowHeight(0, 100);
    QCOMPARE(dummy.rowOffset(0), 0);
    QCOMPARE(dummy.rowOffset(1), 100);
    QCOMPARE(dummy.collapsedRowOffset(1), DefaultRowHeight);
    QCOMPARE(dummy.expandedRowOffset(1), 100);
    QCOMPARE(dummy.expandedRowOffset(2), 100 + DefaultRowHeight);

    dummy.setExpandedRowHeight(1, 50);
    QCOMPARE(dummy.rowOffset(0), 0);
    QCOMPARE(dummy.rowOffset(1), 100);

    // Row heights ignored while collapsed ...
    dummy.setExpanded(false);
    QCOMPARE(dummy.rowOffset(0), 0);
    QCOMPARE(dummy.rowOffset(1), DefaultRowHeight);
    QCOMPARE(dummy.collapsedRowOffset(1), DefaultRowHeight);
    QCOMPARE(dummy.expandedRowOffset(1), 100);
    QCOMPARE(dummy.expandedRowOffset(2), 150);

    // ... but restored when re-expanding
    dummy.setExpanded(true);
    QCOMPARE(dummy.rowOffset(0), 0);
    QCOMPARE(dummy.rowOffset(1), 100);
}

void tst_TimelineModel::height()
{
    DummyModel dummy;
    int heightAfterLastSignal = 0;
    int heightChangedSignals = 0;
    connect(&dummy, &Timeline::TimelineModel::heightChanged, [&](){
        QVERIFY(dummy.height() != heightAfterLastSignal);
        ++heightChangedSignals;
        heightAfterLastSignal = dummy.height();
    });
    QCOMPARE(dummy.height(), 0);

    dummy.loadData();
    QCOMPARE(heightChangedSignals, 1);
    QCOMPARE(heightAfterLastSignal, dummy.height());
    QCOMPARE(dummy.height(), 2 * DefaultRowHeight);

    dummy.setExpanded(true);
    QCOMPARE(heightChangedSignals, 2);
    QCOMPARE(heightAfterLastSignal, dummy.height());
    QCOMPARE(dummy.height(), 3 * DefaultRowHeight);

    dummy.setExpandedRowHeight(1, 80);
    QCOMPARE(heightChangedSignals, 3);
    QCOMPARE(heightAfterLastSignal, dummy.height());
    QCOMPARE(dummy.height(), 2 * DefaultRowHeight + 80);

    dummy.setHidden(true);
    QCOMPARE(heightChangedSignals, 4);
    QCOMPARE(heightAfterLastSignal, dummy.height());
    QCOMPARE(dummy.height(), 0);

    dummy.clear();
    // When clearing the height can change several times in a row.
    QVERIFY(heightChangedSignals > 4);
    QCOMPARE(heightAfterLastSignal, dummy.height());

    dummy.loadData();
    dummy.setExpanded(true);
    QCOMPARE(heightAfterLastSignal, dummy.height());
    QCOMPARE(dummy.rowHeight(1), DefaultRowHeight); // Make sure the row height gets reset.
}

void tst_TimelineModel::count()
{
    DummyModel dummy;
    QCOMPARE(dummy.count(), 0);
    dummy.loadData();
    QCOMPARE(dummy.count(), NumItems);
    QSignalSpy emptySpy(&dummy, SIGNAL(contentChanged()));
    dummy.clear();
    QCOMPARE(emptySpy.count(), 1);
    QCOMPARE(dummy.count(), 0);
}

void tst_TimelineModel::times()
{
    DummyModel dummy;
    dummy.loadData();
    QCOMPARE(dummy.startTime(0), 0);
    QCOMPARE(dummy.duration(0), ItemDuration * 3 + 2);
    QCOMPARE(dummy.endTime(0), ItemDuration * 3 + 2);

}

void tst_TimelineModel::selectionId()
{
    DummyModel dummy;
    dummy.loadData();
    QCOMPARE(dummy.selectionId(0), 4);
}

void tst_TimelineModel::firstLast()
{
    DummyModel dummy;
    QCOMPARE(dummy.firstIndex(0), -1);
    QCOMPARE(dummy.firstIndex(ItemSpacing), -1);
    QCOMPARE(dummy.lastIndex(0), -1);
    QCOMPARE(dummy.lastIndex(ItemSpacing), -1);
    dummy.insert(0, 10, 5); // test special case for only one item
    QCOMPARE(dummy.firstIndex(5), 0);
    QCOMPARE(dummy.lastIndex(5), 0);
    dummy.clear();
    dummy.loadData();
    QCOMPARE(dummy.firstIndex(0), 0);
    QCOMPARE(dummy.firstIndex(ItemSpacing + 1), 0);
    QCOMPARE(dummy.lastIndex(0), -1);
    QCOMPARE(dummy.lastIndex(ItemSpacing + 1), 14);
    QCOMPARE(dummy.firstIndex(ItemDuration * 5000), -1);
    QCOMPARE(dummy.lastIndex(ItemDuration * 5000), NumItems - 1);
}

void tst_TimelineModel::expand()
{
    DummyModel dummy;
    QSignalSpy spy(&dummy, SIGNAL(expandedChanged()));
    QVERIFY(!dummy.expanded());
    dummy.setExpanded(true);
    QVERIFY(dummy.expanded());
    QCOMPARE(spy.count(), 1);
    dummy.setExpanded(true);
    QVERIFY(dummy.expanded());
    QCOMPARE(spy.count(), 1);
    dummy.setExpanded(false);
    QVERIFY(!dummy.expanded());
    QCOMPARE(spy.count(), 2);
    dummy.setExpanded(false);
    QVERIFY(!dummy.expanded());
    QCOMPARE(spy.count(), 2);
}

void tst_TimelineModel::hide()
{
    DummyModel dummy;
    QSignalSpy spy(&dummy, SIGNAL(hiddenChanged()));
    QVERIFY(!dummy.hidden());
    dummy.setHidden(true);
    QVERIFY(dummy.hidden());
    QCOMPARE(spy.count(), 1);
    dummy.setHidden(true);
    QVERIFY(dummy.hidden());
    QCOMPARE(spy.count(), 1);
    dummy.setHidden(false);
    QVERIFY(!dummy.hidden());
    QCOMPARE(spy.count(), 2);
    dummy.setHidden(false);
    QVERIFY(!dummy.hidden());
    QCOMPARE(spy.count(), 2);
}

void tst_TimelineModel::displayName()
{
    QLatin1String name("testest");
    DummyModel dummy(name);
    QSignalSpy spy(&dummy, SIGNAL(displayNameChanged()));
    QCOMPARE(dummy.displayName(), name);
    QCOMPARE(spy.count(), 0);
    dummy.setDisplayName(name);
    QCOMPARE(dummy.displayName(), name);
    QCOMPARE(spy.count(), 0);
    name = QLatin1String("testerei");
    dummy.setDisplayName(name);
    QCOMPARE(dummy.displayName(), name);
    QCOMPARE(spy.count(), 1);
}

void tst_TimelineModel::defaultValues()
{
    Timeline::TimelineModel dummy(12);
    QCOMPARE(dummy.location(0), QVariantMap());
    QCOMPARE(dummy.handlesTypeId(0), false);
    QCOMPARE(dummy.relativeHeight(0), 1.0);
    QCOMPARE(dummy.rowMinValue(0), 0);
    QCOMPARE(dummy.rowMaxValue(0), 0);
    QCOMPARE(dummy.typeId(0), -1);
    QCOMPARE(dummy.color(0), QRgb());
    QCOMPARE(dummy.labels(), QVariantList());
    QCOMPARE(dummy.details(0), QVariantMap());
    QCOMPARE(dummy.collapsedRow(0), 0);
    QCOMPARE(dummy.expandedRow(0), 0);
}

void tst_TimelineModel::row()
{
    DummyModel dummy;
    dummy.loadData();
    QCOMPARE(dummy.row(0), 1);
    dummy.setExpanded(true);
    QCOMPARE(dummy.row(0), 2);
}

void tst_TimelineModel::colorByHue()
{
    DummyModel dummy;
    QCOMPARE(dummy.colorByHue(10), QColor::fromHsl(10, 150, 166).rgb());
    QCOMPARE(dummy.colorByHue(500), QColor::fromHsl(140, 150, 166).rgb());
}

void tst_TimelineModel::colorBySelectionId()
{
    DummyModel dummy;
    dummy.loadData();
    QCOMPARE(dummy.colorBySelectionId(5), QColor::fromHsl(6 * 25, 150, 166).rgb());
}

void tst_TimelineModel::colorByFraction()
{
    DummyModel dummy;
    QCOMPARE(dummy.colorByFraction(0.5), QColor::fromHsl(0.5 * 96 + 10, 150, 166).rgb());
}

void tst_TimelineModel::supportedRenderPasses()
{
    DummyModel dummy;
    QVERIFY(!dummy.supportedRenderPasses().isEmpty());
}

void tst_TimelineModel::insertStartEnd()
{
    DummyModel dummy;
    int id = dummy.insertStart(10, 0);
    dummy.insertEnd(id, 10);
    QCOMPARE(dummy.startTime(id), 10);
    QCOMPARE(dummy.endTime(id), 20);
    int id2 = dummy.insertStart(5, 3);
    dummy.insertEnd(id2, 10);
    QCOMPARE(dummy.startTime(id2), 5);
    QCOMPARE(dummy.endTime(id2), 15);

    int id3 = dummy.insertStart(10, 1);
    QVERIFY(id3 > id);
    dummy.insertEnd(id3, 20);

    // Make sure that even though the new range is larger than the one pointed to by id, we still
    // get to search from id when looking up a timestamp which is in both ranges.
    QCOMPARE(dummy.firstIndex(11), id);
}

void tst_TimelineModel::rowCount()
{
    DummyModel dummy;
    QSignalSpy expandedSpy(&dummy, SIGNAL(expandedRowCountChanged()));
    QSignalSpy collapsedSpy(&dummy, SIGNAL(collapsedRowCountChanged()));
    QCOMPARE(dummy.rowCount(), 1);
    dummy.setExpanded(true);
    QCOMPARE(dummy.rowCount(), 1);
    dummy.loadData();
    QCOMPARE(expandedSpy.count(), 1);
    QCOMPARE(collapsedSpy.count(), 1);
    QCOMPARE(dummy.rowCount(), 3);
    dummy.setExpanded(false);
    QCOMPARE(dummy.rowCount(), 2);
    dummy.clear();
    QCOMPARE(expandedSpy.count(), 2);
    QCOMPARE(collapsedSpy.count(), 2);
    QCOMPARE(dummy.rowCount(), 1);
}

void tst_TimelineModel::prevNext()
{
    DummyModel dummy;
    QCOMPARE(dummy.nextItemBySelectionId(5, 10, 5), -1);
    QCOMPARE(dummy.prevItemBySelectionId(5, 10, 5), -1);

    dummy.loadData();
    QCOMPARE(dummy.nextItemBySelectionId(5, 5000 * ItemSpacing, -1), 3);
    QCOMPARE(dummy.prevItemBySelectionId(5, 5000 * ItemSpacing, -1), 28);

    QCOMPARE(dummy.nextItemBySelectionId(15, 10, -1), -1);
    QCOMPARE(dummy.prevItemBySelectionId(15, 10, -1), -1);

    QCOMPARE(dummy.nextItemBySelectionId(5, 10, -1), 15);
    QCOMPARE(dummy.prevItemBySelectionId(5, 10, -1), 6);
    QCOMPARE(dummy.nextItemBySelectionId(5, 10, 5), 6);
    QCOMPARE(dummy.prevItemBySelectionId(5, 10, 5), 4);
    QCOMPARE(dummy.nextItemByTypeId(-1, 10, 5), 6);
    QCOMPARE(dummy.prevItemByTypeId(-1, 10, 5), 4);
}

void tst_TimelineModel::parentingOfEqualStarts()
{
    DummyModel dummy;
    // Trick it so that it cannot reorder the events and has to parent them in the "wrong" way ...
    QCOMPARE(dummy.insert(1, 10, 998), 0);
    QCOMPARE(dummy.insertStart(1, 999), 1);
    dummy.insertEnd(1, 20);
    dummy.computeNesting();

    // .. which is reflected in the resulting order.
    QCOMPARE(dummy.selectionId(0), 998);
    QCOMPARE(dummy.selectionId(1), 999);
    QCOMPARE(dummy.firstIndex(10), 0);
    QCOMPARE(dummy.lastIndex(2), 1);
}

QTEST_MAIN(tst_TimelineModel)

#include "tst_timelinemodel.moc"
