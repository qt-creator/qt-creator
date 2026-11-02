// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "quick3dframemodel_test.h"

#include <tracing/timelineformatdata.h>

#include <QTest>

using namespace QmlDebug;

using namespace std::chrono;

namespace Profiler::Internal {

Quick3DFrameModelTest::Quick3DFrameModelTest()
    : model(&manager)
{}

void Quick3DFrameModelTest::testNegativeFrameDelta()
{
    manager.initialize();
    manager.decreaseTraceStart(0ns);
    manager.increaseTraceEnd(2000ns);

    const auto appendEvent = [this](Quick3DFrameModel::MessageType type, nanoseconds timestamp,
                                    const QList<quint64> &numbers) {
        const int typeIndex = manager.numEventTypes();
        manager.appendEventType(QmlEventType(Quick3DEvent, UndefinedRangeType, type));
        QmlEvent event;
        event.setTypeIndex(typeIndex);
        event.setTimestamp(timestamp);
        event.setNumbers<QList<quint64>, quint64>(numbers);
        manager.appendEvent(std::move(event));
    };

    // An event data id is marked in the upper 32 bits of a number.
    const quint64 view3D = quint64(0xed000001) << 32;
    appendEvent(Quick3DFrameModel::EventData, 1ns, {});
    appendEvent(Quick3DFrameModel::SynchronizeFrame, 600ns, {0, 0, view3D});
    appendEvent(Quick3DFrameModel::PrepareFrame, 700ns, {0, 0, view3D});
    appendEvent(Quick3DFrameModel::RenderFrame, 1000ns, {0, 0, view3D});
    // The second frame of the same View3D ends before the first one.
    appendEvent(Quick3DFrameModel::SynchronizeFrame, 10ns, {0, 0, view3D});
    appendEvent(Quick3DFrameModel::PrepareFrame, 15ns, {0, 0, view3D});
    appendEvent(Quick3DFrameModel::RenderFrame, 20ns, {0, 0, view3D});

    manager.finalize();

    QCOMPARE(model.rowCount({}), 2);
    int deltaRows = 0;
    for (int row = 0; row < model.rowCount({}); ++row) {
        const QModelIndex index = model.index(row, Quick3DFrameModel::FrameDelta);
        const QVariant sortValue = model.data(index, Quick3DFrameModel::SortRole);
        if (!sortValue.isValid())
            continue;
        ++deltaRows;
        // The frame groups end at 1001 ns and 21 ns.
        QCOMPARE(sortValue.toLongLong(), qint64(-980));
        const QString label = model.data(index, Qt::DisplayRole).toString();
        QCOMPARE(label, Timeline::formatTime(-980ns));
        QVERIFY(label.startsWith('-'));
    }
    QCOMPARE(deltaRows, 1);
}

} // namespace Profiler::Internal
