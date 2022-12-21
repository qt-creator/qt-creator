// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilertimelinemodel.h"
#include "qmlprofilereventtypes.h"
#include "qmleventlocation.h"

#include <QVariantList>
#include <QColor>
#include <QObject>

namespace QmlProfiler {
class QmlProfilerModelManager;

namespace Internal {

class QmlProfilerAnimationsModel : public QmlProfilerTimelineModel
{
    Q_OBJECT
public:

    struct Item {
        int framerate;
        int animationcount;
        int typeId;
    };

    QmlProfilerAnimationsModel(QmlProfilerModelManager *manager,
                               Timeline::TimelineModelAggregator *parent);

    qint64 rowMaxValue(int rowNumber) const override;

    int typeId(int index) const override;
    Q_INVOKABLE int expandedRow(int index) const override;
    Q_INVOKABLE int collapsedRow(int index) const override;

    QRgb color(int index) const override;
    float relativeHeight(int index) const override;

    QVariantList labels() const override;
    QVariantMap details(int index) const override;

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override;
    void finalize() override;
    void clear() override;

private:
    QVector<Item> m_data;
    int m_maxGuiThreadAnimations = 0;
    int m_maxRenderThreadAnimations = 0;
    qint64 m_minNextStartTimes[2];

    int rowFromThreadId(int threadId) const;
};

} // namespace Internal
} // namespace QmlProfiler
