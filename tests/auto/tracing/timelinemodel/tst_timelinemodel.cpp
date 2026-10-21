// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QColor>
#include <QHash>
#include <QLocale>
#include <QRandomGenerator>
#include <QSignalSpy>
#include <QTest>
#include <tracing/timelineformatdata.h>
#include <tracing/timelinemodel_p.h>
#include <tracing/timelinemodelaggregator.h>
#include <utils/stylehelper.h>

#include <functional>

using namespace Timeline;

static QRgb hueColor(double hue)
{
    return Utils::StyleHelper::oklchColor({defaultColorLightness(), kDefaultChroma, hue}).rgb();
}

static const int NumItems = 32;
static const qint64 ItemDuration = 1 << 19;
static const qint64 ItemSpacing = 1 << 20;

class DummyModel : public TimelineModel
{
    Q_OBJECT
    friend class tst_TimelineModel;
public:
    DummyModel(TimelineModelAggregator *parent);
    DummyModel(QString displayName, TimelineModelAggregator *parent);
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
    void summaries();
    void noSummariesForFewItems();
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
    void numberFormat();
    void unitFormat();
    void row();
    void colorByHue();
    void colorBySelectionId();
    void insertStartEnd();
    void rowCount();
    void prevNext();
    void parentingOfEqualStarts();
    void rows();
    void longRangesAreFoundInsideTheView();
    void longRangesAreFoundAmongOverlappingRanges();

private:
    TimelineModelAggregator aggregator;
};

DummyModel::DummyModel(TimelineModelAggregator *parent) :
    TimelineModel(parent)
{
}

DummyModel::DummyModel(QString displayName, TimelineModelAggregator *parent) :
    TimelineModel(parent)
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
    emit contentChanged();
}

tst_TimelineModel::tst_TimelineModel() :
    DefaultRowHeight(TimelineModel::defaultRowHeight())
{
}

void tst_TimelineModel::isEmpty()
{
    DummyModel dummy(&aggregator);
    QVERIFY(dummy.isEmpty());
    dummy.loadData();
    QVERIFY(!dummy.isEmpty());
    dummy.clear();
    QVERIFY(dummy.isEmpty());
}

void tst_TimelineModel::modelId()
{
    DummyModel dummy(&aggregator);
    QCOMPARE(dummy.modelId(), aggregator.generateModelId() - 1);
}

void tst_TimelineModel::rowHeight()
{
    DummyModel dummy(&aggregator);
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

    QSignalSpy expandedSpy(&dummy, &TimelineModel::expandedRowHeightChanged);
    dummy.clear();
    QCOMPARE(expandedSpy.count(), 1);
}

void tst_TimelineModel::rowOffset()
{
    DummyModel dummy(&aggregator);
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
    DummyModel dummy(&aggregator);
    int heightAfterLastSignal = 0;
    int heightChangedSignals = 0;
    connect(&dummy, &TimelineModel::heightChanged, [&] {
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
    DummyModel dummy(&aggregator);
    QCOMPARE(dummy.count(), 0);
    dummy.loadData();
    QCOMPARE(dummy.count(), NumItems);
    QSignalSpy emptySpy(&dummy, &TimelineModel::contentChanged);
    dummy.clear();
    QCOMPARE(emptySpy.count(), 1);
    QCOMPARE(dummy.count(), 0);
}

void tst_TimelineModel::times()
{
    DummyModel dummy(&aggregator);
    dummy.loadData();
    QCOMPARE(dummy.startTime(0), 0);
    QCOMPARE(dummy.duration(0), ItemDuration * 3 + 2);
    QCOMPARE(dummy.endTime(0), ItemDuration * 3 + 2);

}

void tst_TimelineModel::selectionId()
{
    DummyModel dummy(&aggregator);
    dummy.loadData();
    QCOMPARE(dummy.selectionId(0), 4);
}

void tst_TimelineModel::firstLast()
{
    DummyModel dummy(&aggregator);
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
    DummyModel dummy(&aggregator);
    QSignalSpy spy(&dummy, &TimelineModel::expandedChanged);
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
    DummyModel dummy(&aggregator);
    QSignalSpy spy(&dummy, &TimelineModel::hiddenChanged);
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
    DummyModel dummy(name, &aggregator);
    QSignalSpy spy(&dummy, &TimelineModel::displayNameChanged);
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
    TimelineModel dummy(&aggregator);
    QCOMPARE(dummy.location(0), ItemLocation());
    QCOMPARE(dummy.handlesTypeId(0), false);
    QCOMPARE(dummy.relativeHeight(0), 1.0);
    QCOMPARE(dummy.rowMinValue(0), 0);
    QCOMPARE(dummy.rowMaxValue(0), 0);
    QCOMPARE(dummy.formatRowValue(0, 42), formatNumber(42));
    QCOMPARE(dummy.typeId(0), -1);
    QCOMPARE(dummy.color(0), QRgb());
    QCOMPARE(dummy.labels(), (RowLabels()));
    const Timeline::ItemDetails emptyDetails;
    QCOMPARE(dummy.details(0), emptyDetails);
    QCOMPARE(dummy.collapsedRow(0), 0);
    QCOMPARE(dummy.expandedRow(0), 0);
}

void tst_TimelineModel::numberFormat()
{
    if (QLocale::system().zeroDigit() != u"0")
        QSKIP("The system locale does not use Latin digits.");
    QCOMPARE(formatNumber(0), QString("0"));
    QCOMPARE(formatNumber(42), QString("42"));
}

void tst_TimelineModel::unitFormat()
{
    if (formatNumber(12800) == QString::number(12800))
        QSKIP("The system locale formats numbers like QString::number().");
    QCOMPARE(formatPercentage(12800), formatNumber(12800) + '%');
    QCOMPARE(formatPixels(12800), formatNumber(12800) + " px");
}

void tst_TimelineModel::row()
{
    DummyModel dummy(&aggregator);
    dummy.loadData();
    QCOMPARE(dummy.row(0), 1);
    dummy.setExpanded(true);
    QCOMPARE(dummy.row(0), 2);
}

void tst_TimelineModel::colorByHue()
{
    TimelineModelAggregator aggregator;
    DummyModel dummy(&aggregator);
    QCOMPARE(dummy.colorByHue(10), hueColor(10));
    QCOMPARE(dummy.colorByHue(500), hueColor(140));
    // A caller deriving a hue from something that is not one -- a hash of the
    // process a lane belongs to, say -- can arrive at a negative number. It
    // wraps the same way the one above does, rather than reading a colour from
    // outside the table.
    QCOMPARE(dummy.colorByHue(-220), hueColor(140));
}

void tst_TimelineModel::colorBySelectionId()
{
    DummyModel dummy(&aggregator);
    dummy.loadData();
    QCOMPARE(dummy.colorBySelectionId(5), hueColor(6 * 25));
}

void tst_TimelineModel::insertStartEnd()
{
    DummyModel dummy(&aggregator);
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
    DummyModel dummy(&aggregator);
    QSignalSpy contentSpy(&dummy, &TimelineModel::contentChanged);
    QCOMPARE(dummy.rowCount(), 1);
    dummy.setExpanded(true);
    QCOMPARE(dummy.rowCount(), 1);
    dummy.loadData();
    QCOMPARE(contentSpy.count(), 1);
    QCOMPARE(dummy.rowCount(), 3);
    dummy.setExpanded(false);
    QCOMPARE(dummy.rowCount(), 2);
    dummy.clear();
    QCOMPARE(contentSpy.count(), 2);
    QCOMPARE(dummy.rowCount(), 1);
}

void tst_TimelineModel::prevNext()
{
    DummyModel dummy(&aggregator);
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
    DummyModel dummy(&aggregator);
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

void tst_TimelineModel::rows()
{
    DummyModel dummy(&aggregator);
    dummy.loadData();
    int maxlevel;
    const QList<int> levels = dummy.computeRows(&maxlevel);
    QCOMPARE(levels.at(0), 0);
    QCOMPARE(levels.at(7), 7);
    QCOMPARE(levels.at(10), 2);
    QCOMPARE(maxlevel, 15);
}

// Every range that intersects a view must lie between firstIndex() and
// lastIndex() of that view, however long it is and whether or not its start or
// end is in view.
void tst_TimelineModel::longRangesAreFoundInsideTheView()
{
    DummyModel dummy(&aggregator);
    // One range spanning everything, a nested chain, short leaves at various
    // depths, a range starting together with its parent, and a sibling after.
    dummy.insert(0, 10000, 1);
    dummy.insert(100, 8900, 2);
    dummy.insert(200, 7800, 3);
    dummy.insert(200, 50, 4);
    for (int i = 0; i < 40; ++i)
        dummy.insert(300 + i * 150, 20, 5);
    for (int i = 0; i < 10; ++i)
        dummy.insert(6500 + i * 200, 100, 6);
    dummy.insert(9100, 100, 7);
    dummy.insert(10500, 300, 8);
    dummy.computeNesting();

    int uncovered = 0;
    QString firstFailure;
    for (qint64 width : {1, 7, 60, 400}) {
        for (qint64 start = -50; start < 11000; start += 13) {
            const qint64 end = start + width;
            const int first = dummy.firstIndex(start);
            const int last = dummy.lastIndex(end);
            for (int i = 0; i < dummy.count(); ++i) {
                const bool visible = dummy.startTime(i) < end && dummy.endTime(i) > start;
                if (visible && (first < 0 || i < first || i > last)) {
                    if (firstFailure.isEmpty()) {
                        firstFailure = QString("range %1 [%2, %3] not in view [%4, %5]: %6..%7")
                                           .arg(i).arg(dummy.startTime(i)).arg(dummy.endTime(i))
                                           .arg(start).arg(end).arg(first).arg(last);
                    }
                    ++uncovered;
                }
            }
        }
    }
    QVERIFY2(uncovered == 0, qPrintable(QString("%1 misses, first: %2")
                                            .arg(uncovered).arg(firstFailure)));
}

// Producers of Chrome trace events can emit ranges that overlap without one
// containing the other, which the model documents as unsupported. A long range
// must still show up for a view in its middle.
void tst_TimelineModel::longRangesAreFoundAmongOverlappingRanges()
{
    DummyModel dummy(&aggregator);
    QRandomGenerator random(42);
    dummy.insert(0, 10000, 1);
    for (int i = 0; i < 300; ++i)
        dummy.insert(qint64(random.bounded(10500)) - 200, 1 + random.bounded(900), 2);
    dummy.computeNesting();

    int uncovered = 0;
    QString firstFailure;
    for (qint64 width : {1, 7, 60, 400}) {
        for (qint64 start = -300; start < 11000; start += 11) {
            const qint64 end = start + width;
            const int first = dummy.firstIndex(start);
            const int last = dummy.lastIndex(end);
            for (int i = 0; i < dummy.count(); ++i) {
                const bool visible = dummy.startTime(i) < end && dummy.endTime(i) > start;
                if (visible && (first < 0 || i < first || i > last)) {
                    if (firstFailure.isEmpty()) {
                        firstFailure = QString("range %1 [%2, %3] not in view [%4, %5]: %6..%7")
                                           .arg(i).arg(dummy.startTime(i)).arg(dummy.endTime(i))
                                           .arg(start).arg(end).arg(first).arg(last);
                    }
                    ++uncovered;
                }
            }
        }
    }
    QVERIFY2(uncovered == 0, qPrintable(QString("%1 misses, first: %2")
                                            .arg(uncovered).arg(firstFailure)));
}

// Overlapping items of two selection ids in collapsed row 0 and three expanded rows, of varying
// durations so that buckets hold competing items.
class SummarizedModel : public TimelineModel
{
public:
    SummarizedModel(TimelineModelAggregator *parent) : TimelineModel(parent) {}

    void loadData(int items)
    {
        setCollapsedRowCount(1);
        setExpandedRowCount(3);
        for (int i = 0; i < items; ++i)
            insert(i * 7, 1 + (i * 13) % 29, i % 2);
        computeSummaries();
    }

    void add(qint64 start, qint64 duration, int selectionId)
    {
        insert(start, duration, selectionId);
    }

    int expandedRow(int index) const override { return index % 3; }
    int collapsedRow(int) const override { return 0; }
};

// For every bucket of every row, the item overlapping it most, by brute force.
static void verifySummary(const TimelineModel &model, const TimelineModel::Summary &summary,
                          const std::function<int(int)> &rowOf)
{
    QCOMPARE(summary.start, model.startTime(0));
    for (int row = 0; row < summary.rows.size(); ++row) {
        QHash<quint32, std::pair<int, qint64>> expected;
        for (int i = 0; i < model.count(); ++i) {
            if (rowOf(i) != row)
                continue;
            const qint64 start = model.startTime(i) - summary.start;
            const qint64 end = model.endTime(i) - summary.start;
            const qint64 bucketDuration = summary.bucketDuration;
            for (qint64 b = start / bucketDuration; b <= end / bucketDuration; ++b) {
                const qint64 overlap = std::min(end, (b + 1) * bucketDuration)
                                       - std::max(start, b * bucketDuration);
                auto it = expected.find(quint32(b));
                if (it == expected.end() || overlap > it->second)
                    expected[quint32(b)] = {i, overlap};
            }
        }
        const QList<TimelineModel::Summary::Bucket> &buckets = summary.rows.at(row);
        QCOMPARE(buckets.size(), expected.size());
        for (int b = 0; b < buckets.size(); ++b) {
            if (b > 0)
                QVERIFY(buckets.at(b - 1).index < buckets.at(b).index);
            const auto it = expected.constFind(buckets.at(b).index);
            QVERIFY(it != expected.constEnd());
            QCOMPARE(buckets.at(b).overlap, float(it->second));
            QCOMPARE(buckets.at(b).item, it->first);
        }
    }
}

void tst_TimelineModel::summaries()
{
    TimelineModelAggregator aggregator;
    SummarizedModel model(&aggregator);
    model.loadData(70000);

    const TimelineModel::Summary *collapsed = model.summary(false);
    const TimelineModel::Summary *expanded = model.summary(true);
    const TimelineModel::Summary *overview = model.overviewSummary();
    QVERIFY(collapsed && expanded && overview);
    QCOMPARE(collapsed->rows.size(), 1);
    QCOMPARE(expanded->rows.size(), 3);
    QCOMPARE(overview->rows.size(), 1);
    QVERIFY(collapsed->bucketDuration > 1);
    QCOMPARE(collapsed->mixedRows, QList<bool>{true});
    // Row r holds the items i with i % 3 == r, so every row has both selection ids.
    QCOMPARE(expanded->mixedRows, QList<bool>({true, true, true}));

    verifySummary(model, *collapsed, [](int) { return 0; });
    if (QTest::currentTestFailed())
        return;
    verifySummary(model, *expanded, [](int i) { return i % 3; });

    // Items added later are not in the summaries, so they go.
    model.add(1000000, 10, 0);
    QVERIFY(!model.summary(false));
    QVERIFY(!model.summary(true));
    QVERIFY(!model.overviewSummary());
}

void tst_TimelineModel::noSummariesForFewItems()
{
    TimelineModelAggregator aggregator;
    SummarizedModel model(&aggregator);
    model.loadData(1000);
    QVERIFY(!model.summary(false));
    QVERIFY(!model.overviewSummary());
}

QTEST_GUILESS_MAIN(tst_TimelineModel)

#include "tst_timelinemodel.moc"
