// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilertimelinemodel.h"
#include "qmlprofilereventtypes.h"
#include "qmleventlocation.h"
#include "qmlprofilerconstants.h"

#include <QVariantList>
#include <QColor>
#include <QStack>

namespace QmlProfiler {
class QmlProfilerModelManager;

namespace Internal {

class QmlProfilerRangeModel : public QmlProfilerTimelineModel
{
    Q_OBJECT
public:

    struct Item {
        // not-expanded, per type
        int displayRowExpanded = 1;
        int displayRowCollapsed = Constants::QML_MIN_LEVEL;
        int bindingLoopHead = -1;
    };

    QmlProfilerRangeModel(QmlProfilerModelManager *manager, RangeType range,
                          Timeline::TimelineModelAggregator *parent);

    Q_INVOKABLE int expandedRow(int index) const override;
    Q_INVOKABLE int collapsedRow(int index) const override;
    int bindingLoopDest(int index) const;
    QRgb color(int index) const override;

    QVariantList labels() const override;
    QVariantMap details(int index) const override;
    QVariantMap location(int index) const override;

    int typeId(int index) const override;

    QList<const Timeline::TimelineRenderPass *> supportedRenderPasses() const override;

protected:
    void loadEvent(const QmlEvent &event, const QmlEventType &type) override;
    void finalize() override;
    void clear() override;

private:

    bool supportsBindingLoops() const;
    void computeNestingContracted();
    void computeExpandedLevels();
    void findBindingLoops();

    QVector<Item> m_data;
    QStack<int> m_stack;
    QVector<int> m_expandedRowTypes;
};

} // namespace Internal
} // namespace QmlProfiler
