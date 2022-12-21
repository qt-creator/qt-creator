// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofiler_global.h"
#include "qmlprofilermodelmanager.h"

#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>

namespace QmlProfiler {

class QMLPROFILER_EXPORT QmlProfilerTimelineModel : public Timeline::TimelineModel {
    Q_OBJECT
    Q_PROPERTY(RangeType rangeType READ rangeType CONSTANT)
    Q_PROPERTY(Message message READ message CONSTANT)
    Q_PROPERTY(QmlProfilerModelManager *modelManager READ modelManager CONSTANT)

public:
    QmlProfilerTimelineModel(QmlProfilerModelManager *modelManager, Message message,
                             RangeType rangeType, ProfileFeature mainFeature,
                             Timeline::TimelineModelAggregator *parent);

    QmlProfilerModelManager *modelManager() const;

    RangeType rangeType() const;
    Message message() const;
    ProfileFeature mainFeature() const;

    bool handlesTypeId(int typeId) const override;

    QVariantMap locationFromTypeId(int index) const;

    virtual void loadEvent(const QmlEvent &event, const QmlEventType &type) = 0;
    virtual void initialize();
    virtual void finalize();

private:
    void onVisibleFeaturesChanged(quint64 features);

    const Message m_message;
    const RangeType m_rangeType;
    const ProfileFeature m_mainFeature;
    QmlProfilerModelManager *const m_modelManager;

    void updateProgress(qint64 count, qint64 max) const;
};

} // namespace QmlProfiler
