// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilernotesmodel_test.h"

#include "../qmlprofilernotesmodel.h"
#include "../qmlprofilerrangemodel.h"

#include <QTest>

using namespace QmlDebug;

namespace Profiler::Internal {

QmlProfilerNotesModelTest::QmlProfilerNotesModelTest() = default;

static QmlProfilerNotesModel *notesModel(QmlProfilerModelManager *manager)
{
    return static_cast<QmlProfilerNotesModel *>(manager->notesModel());
}

static void appendRange(QmlProfilerModelManager *manager, int typeIndex, qint64 start, qint64 end)
{
    QmlEvent event;
    event.setTypeIndex(typeIndex);
    event.setTimestamp(start);
    event.setRangeStage(RangeStart);
    manager->appendEvent(QmlEvent(event));
    event.setTimestamp(end);
    event.setRangeStage(RangeEnd);
    manager->appendEvent(std::move(event));
}

// The inner range starts at the same time as the outer one, but lasts shorter.
// Both have the same type. Only the row tells them apart for a note that has the
// outer range's start and duration.
void QmlProfilerNotesModelTest::initTestCase()
{
    m_rangeModel = new QmlProfilerRangeModel(&m_manager, Javascript, &m_aggregator);
    notesModel(&m_manager)->addTimelineModel(m_rangeModel);

    m_manager.initialize();
    const int typeIndex = m_manager.appendEventType(
        QmlEventType(UndefinedMessage, Javascript, -1, QmlEventLocation("somefile.js", 1, 1),
                     QString("funcfunc")));
    appendRange(&m_manager, typeIndex, 1, 10);
    appendRange(&m_manager, typeIndex, 1, 9);
    m_manager.finalize();

    QCOMPARE(m_rangeModel->count(), 2);
    for (int i = 0; i < m_rangeModel->count(); ++i) {
        if (m_rangeModel->collapsedRow(i) == 1)
            m_innerIndex = i;
    }
    QVERIFY(m_innerIndex != -1);
    QCOMPARE(m_rangeModel->duration(m_innerIndex), 8);
}

void QmlProfilerNotesModelTest::testStashSavesRowsFromOne()
{
    QmlProfilerNotesModel *model = notesModel(&m_manager);
    model->clear();
    QVERIFY(model->add(m_rangeModel->modelId(), m_innerIndex, "inner") != -1);

    model->stash();

    QCOMPARE(model->notes().count(), 1);
    QCOMPARE(model->notes().first().collapsedRow(), 2);
}

void QmlProfilerNotesModelTest::testRestoreReadsRowsFromOne()
{
    QmlProfilerNotesModel *model = notesModel(&m_manager);
    model->clear();
    // Saved row 2 is collapsed row 1, although start time and duration are those of row 0.
    model->setNotes({QmlNote(0, 2, 1, 9, "inner")});

    model->restore();

    QCOMPARE(model->count(), 1);
    QCOMPARE(model->timelineModel(0), m_rangeModel->modelId());
    QCOMPARE(model->timelineIndex(0), m_innerIndex);
    QCOMPARE(model->text(0), QString("inner"));
}

void QmlProfilerNotesModelTest::testRestoreWithoutRow()
{
    QmlProfilerNotesModel *model = notesModel(&m_manager);
    model->clear();
    model->setNotes({QmlNote(0, -1, 1, 9, "any row")});

    model->restore();

    QCOMPARE(model->count(), 1);
    QCOMPARE(model->timelineModel(0), m_rangeModel->modelId());
    QCOMPARE(m_rangeModel->duration(model->timelineIndex(0)), 9);
}

} // namespace Profiler::Internal
