// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelineabstractrenderer_p.h>

using namespace Timeline;

class DummyRenderer : public TimelineAbstractRenderer {
    friend class tst_TimelineAbstractRenderer;
public:
    DummyRenderer() : TimelineAbstractRenderer(*new TimelineAbstractRendererPrivate) {}
};

class tst_TimelineAbstractRenderer : public QObject
{
    Q_OBJECT

private slots:
    void privateCtor();
    void selectionLocked();
    void selectedItem();
    void model();
    void notes();
    void zoomer();
    void dirty();
};

void tst_TimelineAbstractRenderer::privateCtor()
{
    DummyRenderer renderer;
    QVERIFY(renderer.d_func() != 0);
}

void tst_TimelineAbstractRenderer::selectionLocked()
{
    TimelineAbstractRenderer renderer;
    QSignalSpy spy(&renderer, &TimelineAbstractRenderer::selectionLockedChanged);
    QCOMPARE(spy.count(), 0);
    QVERIFY(renderer.selectionLocked());
    renderer.setSelectionLocked(false);
    QCOMPARE(spy.count(), 1);
    QVERIFY(!renderer.selectionLocked());
    renderer.setSelectionLocked(true);
    QCOMPARE(spy.count(), 2);
    QVERIFY(renderer.selectionLocked());
}

void tst_TimelineAbstractRenderer::selectedItem()
{
    TimelineAbstractRenderer renderer;
    QSignalSpy spy(&renderer, &TimelineAbstractRenderer::selectedItemChanged);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(renderer.selectedItem(), -1);
    renderer.setSelectedItem(12);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(renderer.selectedItem(), 12);
}

void tst_TimelineAbstractRenderer::model()
{
    TimelineAbstractRenderer renderer;
    TimelineModelAggregator aggregator;
    QSignalSpy spy(&renderer, &TimelineAbstractRenderer::modelChanged);
    QVERIFY(!renderer.modelDirty());
    QCOMPARE(spy.count(), 0);
    TimelineModel model(&aggregator);
    QCOMPARE(renderer.model(), static_cast<TimelineModel *>(0));
    renderer.setModel(&model);
    QVERIFY(renderer.modelDirty());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(renderer.model(), &model);
    renderer.setModel(0);
    QVERIFY(renderer.modelDirty());
    QCOMPARE(spy.count(), 2);
    QCOMPARE(renderer.model(), static_cast<TimelineModel *>(0));
}

void tst_TimelineAbstractRenderer::notes()
{
    TimelineAbstractRenderer renderer;
    QSignalSpy spy(&renderer, &TimelineAbstractRenderer::notesChanged);
    QVERIFY(!renderer.notesDirty());
    QCOMPARE(spy.count(), 0);
    TimelineNotesModel notes;
    QCOMPARE(renderer.notes(), static_cast<TimelineNotesModel *>(0));
    renderer.setNotes(&notes);
    QVERIFY(renderer.notesDirty());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(renderer.notes(), &notes);
    renderer.setNotes(0);
    QVERIFY(renderer.notesDirty());
    QCOMPARE(spy.count(), 2);
    QCOMPARE(renderer.notes(), static_cast<TimelineNotesModel *>(0));
}

void tst_TimelineAbstractRenderer::zoomer()
{
    TimelineAbstractRenderer renderer;
    QSignalSpy spy(&renderer, &TimelineAbstractRenderer::zoomerChanged);
    QCOMPARE(spy.count(), 0);
    TimelineZoomControl zoomer;
    QCOMPARE(renderer.zoomer(), static_cast<TimelineZoomControl *>(0));
    renderer.setZoomer(&zoomer);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(renderer.zoomer(), &zoomer);
    renderer.setZoomer(0);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(renderer.zoomer(), static_cast<TimelineZoomControl *>(0));
}

void tst_TimelineAbstractRenderer::dirty()
{
    DummyRenderer renderer;
    QVERIFY(!renderer.modelDirty());
    QVERIFY(!renderer.notesDirty());
    QVERIFY(!renderer.rowHeightsDirty());

    renderer.setModelDirty();
    QVERIFY(renderer.modelDirty());
    renderer.setNotesDirty();
    QVERIFY(renderer.notesDirty());
    renderer.setRowHeightsDirty();
    QVERIFY(renderer.rowHeightsDirty());

    renderer.updatePaintNode(0, 0);
    QVERIFY(!renderer.modelDirty());
    QVERIFY(!renderer.notesDirty());
    QVERIFY(!renderer.rowHeightsDirty());
}

QTEST_GUILESS_MAIN(tst_TimelineAbstractRenderer)

#include "tst_timelineabstractrenderer.moc"
