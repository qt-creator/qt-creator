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

#include "qmlprofilertimelinemodel.h"

namespace QmlProfiler {

QmlProfilerTimelineModel::QmlProfilerTimelineModel(QmlProfilerModelManager *modelManager,
                                                   Message message, RangeType rangeType,
                                                   ProfileFeature mainFeature,
                                                   Timeline::TimelineModelAggregator *parent) :
    TimelineModel(parent), m_message(message), m_rangeType(rangeType), m_mainFeature(mainFeature),
    m_modelManager(modelManager)
{
    setDisplayName(tr(QmlProfilerModelManager::featureName(mainFeature)));
    connect(modelManager, &QmlProfilerModelManager::typeDetailsFinished,
            this, &Timeline::TimelineModel::labelsChanged);
    connect(modelManager, &QmlProfilerModelManager::typeDetailsFinished,
            this, &Timeline::TimelineModel::detailsChanged);
    connect(modelManager, &QmlProfilerModelManager::visibleFeaturesChanged,
            this, &QmlProfilerTimelineModel::onVisibleFeaturesChanged);

    m_modelManager->registerFeatures(1ULL << m_mainFeature,
                                     std::bind(&QmlProfilerTimelineModel::loadEvent, this,
                                               std::placeholders::_1, std::placeholders::_2),
                                     std::bind(&QmlProfilerTimelineModel::initialize, this),
                                     std::bind(&QmlProfilerTimelineModel::finalize, this),
                                     std::bind(&QmlProfilerTimelineModel::clear, this));
}

RangeType QmlProfilerTimelineModel::rangeType() const
{
    return m_rangeType;
}

Message QmlProfilerTimelineModel::message() const
{
    return m_message;
}

ProfileFeature QmlProfilerTimelineModel::mainFeature() const
{
    return m_mainFeature;
}

bool QmlProfilerTimelineModel::handlesTypeId(int typeIndex) const
{
    if (typeIndex < 0)
        return false;

    return modelManager()->eventType(typeIndex).feature() == m_mainFeature;
}

QmlProfilerModelManager *QmlProfilerTimelineModel::modelManager() const
{
    return m_modelManager;
}

void QmlProfilerTimelineModel::onVisibleFeaturesChanged(quint64 features)
{
    setHidden(!(features & (1ULL << m_mainFeature)));
}

QVariantMap QmlProfilerTimelineModel::locationFromTypeId(int index) const
{
    QVariantMap result;
    int id = typeId(index);
    if (id < 0)
        return result;

    const QmlProfilerModelManager *manager = modelManager();
    if (id >= manager->numEventTypes())
        return result;

    QmlEventLocation location = manager->eventType(id).location();

    result.insert(QStringLiteral("file"), location.filename());
    result.insert(QStringLiteral("line"), location.line());
    result.insert(QStringLiteral("column"), location.column());

    return result;
}

void QmlProfilerTimelineModel::initialize()
{
    setHidden(!(modelManager()->visibleFeatures() & (1ULL << m_mainFeature)));
}

void QmlProfilerTimelineModel::finalize()
{
    emit contentChanged();
}

} // namespace QmlProfiler
