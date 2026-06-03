// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinenotesmodel.h>

#include <QTest>

using namespace Timeline;

class DummyModel : public TimelineModel
{
public:
    DummyModel(TimelineModelAggregator *parent) : TimelineModel(parent) {}
};

class tst_NotesModelFiltering : public QObject
{
    Q_OBJECT

private slots:
    void notesByModel();
    void crossModelIsolation();
    void removeModel();
};

void tst_NotesModelFiltering::notesByModel()
{
    TimelineModelAggregator aggregator;
    DummyModel modelA(&aggregator);
    DummyModel modelB(&aggregator);

    TimelineNotesModel notes;
    notes.addTimelineModel(&modelA);
    notes.addTimelineModel(&modelB);

    notes.add(modelA.modelId(), 0, QLatin1String("note for A"));
    notes.add(modelA.modelId(), 1, QLatin1String("another for A"));
    notes.add(modelB.modelId(), 0, QLatin1String("note for B"));

    QCOMPARE(notes.byTimelineModel(modelA.modelId()).count(), 2);
    QCOMPARE(notes.byTimelineModel(modelB.modelId()).count(), 1);
}

void tst_NotesModelFiltering::crossModelIsolation()
{
    TimelineModelAggregator aggregator;
    DummyModel modelA(&aggregator);
    DummyModel modelB(&aggregator);

    TimelineNotesModel notes;
    notes.addTimelineModel(&modelA);
    notes.addTimelineModel(&modelB);

    notes.add(modelA.modelId(), 0, QLatin1String("A only"));

    QCOMPARE(notes.byTimelineModel(modelA.modelId()).count(), 1);
    QCOMPARE(notes.byTimelineModel(modelB.modelId()).count(), 0);
}

void tst_NotesModelFiltering::removeModel()
{
    TimelineModelAggregator aggregator;
    DummyModel modelA(&aggregator);

    TimelineNotesModel notes;
    notes.addTimelineModel(&modelA);
    notes.add(modelA.modelId(), 0, QLatin1String("note"));
    QCOMPARE(notes.byTimelineModel(modelA.modelId()).count(), 1);

    notes.removeTimelineModel(&modelA);
    QCOMPARE(notes.byTimelineModel(modelA.modelId()).count(), 1);
}

QTEST_GUILESS_MAIN(tst_NotesModelFiltering)

#include "tst_notesmodelfiltering.moc"
