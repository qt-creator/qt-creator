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

#include <QtTest>
#include <qmlprofiler/abstracttimelinemodel.h>
#include <qmlprofiler/abstracttimelinemodel_p.h>

using namespace QmlProfiler;

static const int DefaultRowHeight = 30;
static const int NumItems = 10;
static const qint64 ItemDuration = 1 << 19;
static const qint64 ItemSpacing = 1 << 20;

class DummyModelPrivate;
class DummyModel : public AbstractTimelineModel {
    Q_OBJECT
public:
    Q_DECLARE_PRIVATE(DummyModel)

    DummyModel(QString displayName = tr("dummy"), QObject *parent = 0);
    const QmlProfilerModelManager *modelManager() const;
    int rowCount() const;
    int eventId(int index) const { return index; }
    QColor color(int) const { return QColor(); }
    QVariantList labels() const { return QVariantList(); }
    QVariantMap details(int) const { return QVariantMap(); }
    int row(int) const { return 1; }
    void loadData();


    // Make some protected methods public

    QColor colorByHue(int hue) const
    {
        return AbstractTimelineModel::colorByHue(hue);
    }
    QColor colorByEventId(int index) const
    {
        return AbstractTimelineModel::colorByEventId(index);
    }
    QColor colorByFraction(double fraction) const
    {
        return AbstractTimelineModel::colorByFraction(fraction);
    }
};


class DummyModelPrivate : public AbstractTimelineModel::AbstractTimelineModelPrivate {
public:
    Q_DECLARE_PUBLIC(DummyModel)
};

class tst_AbstractTimelineModel : public QObject
{
    Q_OBJECT

private slots:
    void modelManager();
    void isEmpty();
    void rowHeight();
    void rowOffset();
    void height();
    void traceTime();
    void accepted();
    void expand();
    void displayName();
    void defaultValues();
    void colorByHue();
    void colorByEventId();
    void colorByFraction();
};

DummyModel::DummyModel(QString displayName, QObject *parent) :
    AbstractTimelineModel(new DummyModelPrivate, displayName, QmlDebug::MaximumMessage,
                          QmlDebug::MaximumRangeType, parent)
{
}

const QmlProfilerModelManager *DummyModel::modelManager() const
{
    Q_D(const DummyModel);
    return d->modelManager;
}

int DummyModel::rowCount() const
{
    return isEmpty() ? 1 : 2;
}

void DummyModel::loadData()
{
    for (int i = 0; i < NumItems; ++i)
        insert(i * ItemSpacing, ItemDuration);
}

void tst_AbstractTimelineModel::modelManager()
{
    DummyModel dummy;
    QCOMPARE(dummy.modelManager(), (QmlProfilerModelManager *)0);
    QmlProfilerModelManager manager(0);
    dummy.setModelManager(&manager);
    QCOMPARE(dummy.modelManager(), &manager);
    QmlProfilerModelManager manager2(0);
    dummy.setModelManager(&manager2);
    QCOMPARE(dummy.modelManager(), &manager2);
}

void tst_AbstractTimelineModel::isEmpty()
{
    DummyModel dummy;
    QVERIFY(dummy.isEmpty());
    QmlProfilerModelManager manager(0);
    dummy.setModelManager(&manager);
    dummy.loadData();
    QVERIFY(!dummy.isEmpty());
    dummy.clear();
    QVERIFY(dummy.isEmpty());
}

void tst_AbstractTimelineModel::rowHeight()
{
    DummyModel dummy;
    QCOMPARE(dummy.rowHeight(0), DefaultRowHeight);

    // Cannot set while not expanded
    dummy.setRowHeight(0, 100);
    QCOMPARE(dummy.rowHeight(0), DefaultRowHeight);

    dummy.setExpanded(true);
    QCOMPARE(dummy.rowHeight(0), DefaultRowHeight);

    // Cannot set smaller than default
    dummy.setRowHeight(0, DefaultRowHeight - 1);
    QCOMPARE(dummy.rowHeight(0), DefaultRowHeight);

    dummy.setRowHeight(0, 100);
    QCOMPARE(dummy.rowHeight(0), 100);

    dummy.loadData();
    dummy.setRowHeight(1, 50);
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
}

void tst_AbstractTimelineModel::rowOffset()
{
    DummyModel dummy;
    QCOMPARE(dummy.rowOffset(0), 0);

    dummy.loadData();
    dummy.setExpanded(true);
    QCOMPARE(dummy.rowOffset(0), 0);
    QCOMPARE(dummy.rowOffset(1), DefaultRowHeight);

    dummy.setRowHeight(0, 100);
    QCOMPARE(dummy.rowOffset(0), 0);
    QCOMPARE(dummy.rowOffset(1), 100);

    dummy.setRowHeight(1, 50);
    QCOMPARE(dummy.rowOffset(0), 0);
    QCOMPARE(dummy.rowOffset(1), 100);

    // Row heights ignored while collapsed ...
    dummy.setExpanded(false);
    QCOMPARE(dummy.rowOffset(0), 0);
    QCOMPARE(dummy.rowOffset(1), DefaultRowHeight);

    // ... but restored when re-expanding
    dummy.setExpanded(true);
    QCOMPARE(dummy.rowOffset(0), 0);
    QCOMPARE(dummy.rowOffset(1), 100);
}

void tst_AbstractTimelineModel::height()
{
    DummyModel dummy;
    QCOMPARE(dummy.height(), DefaultRowHeight);
    dummy.loadData();
    QCOMPARE(dummy.height(), 2 * DefaultRowHeight);
    dummy.setExpanded(true);
    QCOMPARE(dummy.height(), 2 * DefaultRowHeight);
    dummy.setRowHeight(0, 80);
    QCOMPARE(dummy.height(), DefaultRowHeight + 80);
}

void tst_AbstractTimelineModel::traceTime()
{
    DummyModel dummy;
    QmlProfilerModelManager manager(0);
    dummy.setModelManager(&manager);
    QCOMPARE(dummy.traceStartTime(), -1);
    QCOMPARE(dummy.traceEndTime(), -1);
    QCOMPARE(dummy.traceDuration(), 0);
}

void tst_AbstractTimelineModel::accepted()
{
    DummyModel dummy;
    QmlProfilerDataModel::QmlEventTypeData event;
    event.message = QmlDebug::MaximumMessage;
    event.rangeType = QmlDebug::MaximumRangeType;
    QVERIFY(dummy.accepted(event));
    event.message = QmlDebug::Event;
    QVERIFY(!dummy.accepted(event));
    event.rangeType = QmlDebug::Painting;
    QVERIFY(!dummy.accepted(event));
    event.message = QmlDebug::MaximumMessage;
    QVERIFY(!dummy.accepted(event));
}

void tst_AbstractTimelineModel::expand()
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

void tst_AbstractTimelineModel::displayName()
{
    QLatin1String name("testest");
    DummyModel dummy(name);
    QCOMPARE(dummy.displayName(), name);
}

void tst_AbstractTimelineModel::defaultValues()
{
    DummyModel dummy;
    dummy.loadData();
    QCOMPARE(dummy.location(0), QVariantMap());
    QCOMPARE(dummy.eventIdForTypeIndex(0), -1);
    QCOMPARE(dummy.eventIdForLocation(QString(), 0, 0), -1);
    QCOMPARE(dummy.bindingLoopDest(0), -1);
    QCOMPARE(dummy.relativeHeight(0), 1.0);
    QCOMPARE(dummy.rowMinValue(0), 0);
    QCOMPARE(dummy.rowMaxValue(0), 0);
}

void tst_AbstractTimelineModel::colorByHue()
{
    DummyModel dummy;
    QCOMPARE(dummy.colorByHue(10), QColor::fromHsl(10, 150, 166));
    QCOMPARE(dummy.colorByHue(500), QColor::fromHsl(140, 150, 166));
}

void tst_AbstractTimelineModel::colorByEventId()
{
    DummyModel dummy;
    dummy.loadData();
    QCOMPARE(dummy.colorByEventId(5), QColor::fromHsl(5 * 25, 150, 166));
}

void tst_AbstractTimelineModel::colorByFraction()
{
    DummyModel dummy;
    QCOMPARE(dummy.colorByFraction(0.5), QColor::fromHsl(0.5 * 96 + 10, 150, 166));
}

QTEST_MAIN(tst_AbstractTimelineModel)

#include "tst_abstracttimelinemodel.moc"
