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

#ifndef QMLPROFILERTIMELINEMODEL_H
#define QMLPROFILERTIMELINEMODEL_H

#include "qmlprofiler_global.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdatamodel.h"
#include "timeline/timelinemodel.h"

namespace QmlProfiler {

class QMLPROFILER_EXPORT QmlProfilerTimelineModel : public Timeline::TimelineModel {
    Q_OBJECT
    Q_PROPERTY(QmlDebug::RangeType rangeType READ rangeType CONSTANT)
    Q_PROPERTY(QmlDebug::Message message READ message CONSTANT)
    Q_PROPERTY(QmlProfilerModelManager *modelManager READ modelManager CONSTANT)

public:
    QmlProfilerTimelineModel(QmlProfilerModelManager *modelManager,
                             QmlDebug::Message message, QmlDebug::RangeType rangeType,
                             QmlDebug::ProfileFeature mainFeature, QObject *parent);

    QmlProfilerModelManager *modelManager() const;

    QmlDebug::RangeType rangeType() const;
    QmlDebug::Message message() const;
    QmlDebug::ProfileFeature mainFeature() const;

    virtual bool accepted(const QmlProfilerDataModel::QmlEventTypeData &event) const;
    bool handlesTypeId(int typeId) const;
    Q_INVOKABLE virtual int bindingLoopDest(int index) const;
    QVariantMap locationFromTypeId(int index) const;

    virtual void loadData() = 0;
    void clear();

private slots:
    void dataChanged();
    void onVisibleFeaturesChanged(quint64 features);

protected:
    void updateProgress(qint64 count, qint64 max) const;
    void announceFeatures(quint64 features) const;

private:
    const QmlDebug::Message m_message;
    const QmlDebug::RangeType m_rangeType;
    const QmlDebug::ProfileFeature m_mainFeature;
    QmlProfilerModelManager *const m_modelManager;
};

}

#endif // QMLPROFILERTIMELINEMODEL_H
