// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertimelinemodel.h"

#include "qmlprofilermodelmanager.h"
#include "qmlprofilertr.h"

using namespace QmlDebug;

namespace QmlProfiler::Internal {

QmlProfilerTimelineModel::QmlProfilerTimelineModel(QmlProfilerModelManager *modelManager,
                                                   Message message, RangeType rangeType,
                                                   ProfileFeature mainFeature,
                                                   Timeline::TimelineModelAggregator *parent) :
    TimelineModel(parent), m_message(message), m_rangeType(rangeType), m_mainFeature(mainFeature),
    m_modelManager(modelManager)
{
    setDisplayName(Tr::tr(QmlProfilerModelManager::featureName(mainFeature)));
    connect(modelManager, &QmlProfilerModelManager::typeDetailsFinished,
            this, &Timeline::TimelineModel::labelsChanged);
    connect(modelManager, &QmlProfilerModelManager::typeDetailsFinished,
            this, &Timeline::TimelineModel::detailsChanged);
    connect(modelManager, &QmlProfilerModelManager::visibleFeaturesChanged,
            this, &QmlProfilerTimelineModel::onVisibleFeaturesChanged);

    m_modelManager->qmlLoaders.append({1ULL << m_mainFeature,
                                       std::bind(&QmlProfilerTimelineModel::loadEvent, this,
                                                 std::placeholders::_1, std::placeholders::_2)});
    m_modelManager->registerFeatures(1ULL << m_mainFeature,
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

Timeline::ItemLocation QmlProfilerTimelineModel::locationFromTypeId(int index) const
{
    int id = typeId(index);
    if (id < 0 || id >= modelManager()->numEventTypes())
        return {};
    const QmlEventLocation location = modelManager()->eventType(id).location();
    return {location.filename(), location.line(), location.column()};
}

void QmlProfilerTimelineModel::initialize()
{
    setHidden(!(modelManager()->visibleFeatures() & (1ULL << m_mainFeature)));
}

void QmlProfilerTimelineModel::finalize()
{
    emit contentChanged();
}

} // namespace QmlProfiler::Internal
