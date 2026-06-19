// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilertimelinemodel.h"
#include "qmlprofilerconstants.h"

#include <qmldebug/qmleventlocation.h>
#include <qmldebug/qmlprofilereventtypes.h>

#include <QColor>
#include <QStack>

namespace Profiler::Internal {

class QmlProfilerModelManager;

class QmlProfilerRangeModel : public QmlProfilerTimelineModel
{
public:
    struct Item {
        // not-expanded, per type
        int displayRowExpanded = 1;
        int displayRowCollapsed = Constants::QML_MIN_LEVEL;
    };

    QmlProfilerRangeModel(QmlProfilerModelManager *manager, QmlDebug::RangeType range,
                          Timeline::TimelineModelAggregator *parent);

    int expandedRow(int index) const final;
    int collapsedRow(int index) const final;
    QRgb color(int index) const final;

    Timeline::RowLabels labels() const final;
    Timeline::ItemDetails details(int index) const final;
    Timeline::ItemLocation location(int index) const final;

    int typeId(int index) const final;

private:
    void loadEvent(const QmlDebug::QmlEvent &event, const QmlDebug::QmlEventType &type) final;
    void finalize() final;
    void clear() final;

    void computeNestingContracted();
    void computeExpandedLevels();

    QList<Item> m_data;
    QStack<int> m_stack;
    QList<int> m_expandedRowTypes;
};

} // namespace Profiler::Internal
