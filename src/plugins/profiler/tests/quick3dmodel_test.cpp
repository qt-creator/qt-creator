// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "quick3dmodel_test.h"

#include <tracing/timelineformatdata.h>

#include <QTest>

using namespace QmlDebug;

namespace Profiler::Internal {

Quick3DModelTest::Quick3DModelTest()
    : model(&manager, &aggregator)
{}

void Quick3DModelTest::initTestCase()
{
    manager.initialize();
    manager.decreaseTraceStart(0);
    manager.increaseTraceEnd(200);

    // Leave out most message types, so that rows and types do not line up.
    const auto appendEvents = [this](Quick3DModel::MessageType type,
                                     std::initializer_list<qint64> data) {
        const int typeIndex = manager.numEventTypes();
        manager.appendEventType(QmlEventType(Quick3DEvent, UndefinedRangeType, type));
        qint64 timestamp = 10 * type;
        for (const qint64 value : data) {
            QmlEvent event;
            event.setTypeIndex(typeIndex);
            event.setTimestamp(++timestamp);
            event.setNumbers<qint64>({1, value});
            manager.appendEvent(std::move(event));
        }
    };
    appendEvents(Quick3DModel::MeshLoad, {100, 200});
    appendEvents(Quick3DModel::TextureLoad, {4096, 8192});
    appendEvents(Quick3DModel::RenderPass, {0});

    manager.finalize();
}

void Quick3DModelTest::testRowMaxValue()
{
    for (int i = 0; i < model.count(); ++i) {
        const qint64 maxValue = model.rowMaxValue(model.expandedRow(i));
        switch (model.selectionId(i)) {
        case Quick3DModel::MeshMemoryConsumption:
            QCOMPARE(maxValue, qint64(200));
            break;
        case Quick3DModel::TextureMemoryConsumption:
            QCOMPARE(maxValue, qint64(8192));
            break;
        case Quick3DModel::RenderPass:
            QCOMPARE(maxValue, qint64(1));
            break;
        default:
            QCOMPARE(maxValue, qint64(0));
        }
    }
}

void Quick3DModelTest::testFormatRowValue()
{
    QCOMPARE(model.expandedRowCount(), 6);
    QSet<int> memoryTypes;
    for (int i = 0; i < model.count(); ++i) {
        const QString label = model.formatRowValue(model.expandedRow(i), 4096);
        const int type = model.selectionId(i);
        if (type == Quick3DModel::MeshMemoryConsumption
            || type == Quick3DModel::TextureMemoryConsumption) {
            QCOMPARE(label, Timeline::formatDataSize(4096));
            memoryTypes.insert(type);
        } else if (type == Quick3DModel::RenderPass) {
            QCOMPARE(label, QString("4096 render pass(es)"));
        } else {
            QCOMPARE(label, Timeline::formatNumber(4096));
        }
    }
    QCOMPARE(memoryTypes.size(), 2);
}

void Quick3DModelTest::cleanupTestCase()
{
    manager.clearAll();
    QCOMPARE(model.count(), 0);
}

} // namespace Profiler::Internal
