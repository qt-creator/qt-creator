// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofiler_global.h"
#include "qmlprofilermodelmanager.h"

#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>

namespace QmlProfiler {

class QMLPROFILER_EXPORT QmlProfilerTimelineModel : public Timeline::TimelineModel
{
    Q_OBJECT
    Q_PROPERTY(QmlDebug::RangeType rangeType READ rangeType CONSTANT)
    Q_PROPERTY(QmlDebug::Message message READ message CONSTANT)
    Q_PROPERTY(QmlProfilerModelManager *modelManager READ modelManager CONSTANT)

public:
    QmlProfilerTimelineModel(QmlProfilerModelManager *modelManager, QmlDebug::Message message,
                             QmlDebug::RangeType rangeType, QmlDebug::ProfileFeature mainFeature,
                             Timeline::TimelineModelAggregator *parent);

    QmlProfilerModelManager *modelManager() const;

    QmlDebug::RangeType rangeType() const;
    QmlDebug::Message message() const;
    QmlDebug::ProfileFeature mainFeature() const;

    bool handlesTypeId(int typeId) const override;

    QVariantMap locationFromTypeId(int index) const;

    virtual void loadEvent(const QmlDebug::QmlEvent &event, const QmlDebug::QmlEventType &type) = 0;
    virtual void initialize();
    virtual void finalize();

private:
    void onVisibleFeaturesChanged(quint64 features);

    const QmlDebug::Message m_message;
    const QmlDebug::RangeType m_rangeType;
    const QmlDebug::ProfileFeature m_mainFeature;
    QmlProfilerModelManager *const m_modelManager;

    void updateProgress(qint64 count, qint64 max) const;
};

} // namespace QmlProfiler
