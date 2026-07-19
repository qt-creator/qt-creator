// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilertimelinemodel.h"

namespace Profiler::Internal {

class QmlProfilerModelManager;

class QmlProfilerAnimationsModel final : public QmlProfilerTimelineModel
{
public:
    struct Item {
        int framerate;
        int animationcount;
        int typeId;
    };

    QmlProfilerAnimationsModel(QmlProfilerModelManager *manager,
                               Timeline::TimelineModelAggregator *parent);

    qint64 rowMaxValue(int rowNumber) const final;

    int typeId(int index) const final;
    int expandedRow(int index) const final;
    int collapsedRow(int index) const final;

    QRgb color(int index) const final;
    float relativeHeight(int index) const final;

    Timeline::RowLabels labels() const final;
    Timeline::ItemDetails details(int index) const final;

    void loadEvent(const QmlDebug::QmlEvent &event, const QmlDebug::QmlEventType &type) final;
    void finalize() final;
    void clear() final;

private:
    QList<Item> m_data;
    int m_maxGuiThreadAnimations = 0;
    int m_maxRenderThreadAnimations = 0;
    qint64 m_minNextStartTimes[2]{};

    int rowFromThreadId(int threadId) const;
};

} // namespace Profiler::Internal
