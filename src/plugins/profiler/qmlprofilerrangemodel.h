// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilertimelinemodel.h"

#include <qmldebug/qmlprofilereventtypes.h>

#include <QColor>
#include <QStack>

namespace Profiler::Internal {

class QmlProfilerModelManager;

class QmlProfilerRangeModel : public QmlProfilerTimelineModel
{
public:
    QmlProfilerRangeModel(QmlProfilerModelManager *manager, QmlDebug::RangeType range,
                          Timeline::TimelineModelAggregator *parent);

    int expandedRow(int index) const final;
    int collapsedRow(int index) const final;
    QRgb color(int index) const final;

    Timeline::RowLabels labels() const final;
    Timeline::ItemDetails details(int index) const final;
    QString itemLabel(int index) const final;
    Timeline::ItemLocation location(int index) const final;

    int typeId(int index) const final;

private:
    void loadEvent(const QmlDebug::QmlEvent &event, const QmlDebug::QmlEventType &type) final;
    void prepare() final;
    void clear() final;

    void computeNestingContracted();
    void computeExpandedLevels();

    // The collapsed row of each range is its nesting level. One byte holds most of them, the
    // deeper ones are kept by index.
    QList<quint8> m_collapsedRows;
    QHash<int, int> m_deepCollapsedRows;
    // The expanded row of each type, by type index, or -1.
    QList<int> m_expandedRowsByType;
    QStack<int> m_stack;
    QList<int> m_expandedRowTypes;
};

} // namespace Profiler::Internal
