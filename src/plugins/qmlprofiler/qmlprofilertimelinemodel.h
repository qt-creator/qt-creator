/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
