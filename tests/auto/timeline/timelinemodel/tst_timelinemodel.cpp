/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <QtTest>
#include <QColor>
#include <timeline/timelinemodel_p.h>

static const int NumItems = 32;
static const qint64 ItemDuration = 1 << 19;
static const qint64 ItemSpacing = 1 << 20;

class DummyModelPrivate : public Timeline::TimelineModel::TimelineModelPrivate {
public:
    DummyModelPrivate(int modelId) :
        Timeline::TimelineModel::TimelineModelPrivate(modelId, QLatin1String("horst"))
    {}
};

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
};

DummyModel::DummyModel(int modelId) :
    Timeline::TimelineModel(*new DummyModelPrivate(modelId), 0)
{
}

DummyModel::DummyModel(QString displayName, QObject *parent) :
    TimelineModel(12, displayName, parent)
{
}

void DummyModel::loadData()
{
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
    QCOMPARE(dummy.height(), 0);
    dummy.loadData();
    QCOMPARE(dummy.height(), 2 * DefaultRowHeight);
    dummy.setExpanded(true);
    QCOMPARE(dummy.height(), 3 * DefaultRowHeight);
    dummy.setExpandedRowHeight(0, 80);
    QCOMPARE(dummy.height(), 2 * DefaultRowHeight + 80);
}

void tst_TimelineModel::count()
{
    DummyModel dummy;
    QCOMPARE(dummy.count(), 0);
    dummy.loadData();
    QCOMPARE(dummy.count(), NumItems);
    QSignalSpy emptySpy(&dummy, SIGNAL(emptyChanged()));
    dummy.clear();
    QCOMPARE(emptySpy.count(), 1);
    QCOMPARE(dummy.count(), 0);
}

void tst_TimelineModel::times()
{
    DummyModel dummy;
    dummy.loadData();
    QCOMPARE(dummy.startTime(0), 0);
    QCOMPARE(dummy.duration(0), ItemDuration + 2);
    QCOMPARE(dummy.endTime(0), ItemDuration + 2);

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
    QCOMPARE(dummy.firstIndex(0), 2);
    QCOMPARE(dummy.firstIndex(ItemSpacing + 1), 2);
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
    QCOMPARE(dummy.displayName(), name);
}

void tst_TimelineModel::defaultValues()
{
    Timeline::TimelineModel dummy(12, QLatin1String("dings"));
    QCOMPARE(dummy.location(0), QVariantMap());
    QCOMPARE(dummy.handlesTypeId(0), false);
    QCOMPARE(dummy.selectionIdForLocation(QString(), 0, 0), -1);
    QCOMPARE(dummy.relativeHeight(0), 1.0);
    QCOMPARE(dummy.rowMinValue(0), 0);
    QCOMPARE(dummy.rowMaxValue(0), 0);
    QCOMPARE(dummy.typeId(0), -1);
    QCOMPARE(dummy.color(0), QColor());
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
    QCOMPARE(dummy.colorByHue(10), QColor::fromHsl(10, 150, 166));
    QCOMPARE(dummy.colorByHue(500), QColor::fromHsl(140, 150, 166));
}

void tst_TimelineModel::colorBySelectionId()
{
    DummyModel dummy;
    dummy.loadData();
    QCOMPARE(dummy.colorBySelectionId(5), QColor::fromHsl(5 * 25, 150, 166));
}

void tst_TimelineModel::colorByFraction()
{
    DummyModel dummy;
    QCOMPARE(dummy.colorByFraction(0.5), QColor::fromHsl(0.5 * 96 + 10, 150, 166));
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
    QCOMPARE(dummy.prevItemBySelectionId(5, 10, 5), 3);
    QCOMPARE(dummy.nextItemByTypeId(-1, 10, 5), 6);
    QCOMPARE(dummy.prevItemByTypeId(-1, 10, 5), 4);
}

QTEST_MAIN(tst_TimelineModel)

#include "tst_timelinemodel.moc"
