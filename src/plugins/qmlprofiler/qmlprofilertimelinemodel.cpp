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
                                                   ProfileFeature mainFeature, QObject *parent) :
    TimelineModel(modelManager->registerModelProxy(), parent),
    m_message(message), m_rangeType(rangeType), m_mainFeature(mainFeature),
    m_modelManager(modelManager)
{
    setDisplayName(tr(QmlProfilerModelManager::featureName(mainFeature)));
    connect(modelManager, &QmlProfilerModelManager::stateChanged,
            this, &QmlProfilerTimelineModel::dataChanged);
    connect(modelManager, &QmlProfilerModelManager::visibleFeaturesChanged,
            this, &QmlProfilerTimelineModel::onVisibleFeaturesChanged);
    announceFeatures(1ULL << m_mainFeature);
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

bool QmlProfilerTimelineModel::accepted(const QmlEventType &type) const
{
    return (type.rangeType() == m_rangeType && type.message() == m_message);
}

bool QmlProfilerTimelineModel::handlesTypeId(int typeIndex) const
{
    if (typeIndex < 0)
        return false;

    return accepted(modelManager()->eventTypes().at(typeIndex));
}

QmlProfilerModelManager *QmlProfilerTimelineModel::modelManager() const
{
    return m_modelManager;
}

void QmlProfilerTimelineModel::announceFeatures(quint64 features)
{
    m_modelManager->announceFeatures(
                features, [this](const QmlEvent &event, const QmlEventType &type) {
        loadEvent(event, type);
    }, [this]() {
        finalize();
    });
}

void QmlProfilerTimelineModel::dataChanged()
{
    switch (m_modelManager->state()) {
    case QmlProfilerModelManager::Done:
        emit contentChanged();
        break;
    case QmlProfilerModelManager::ClearingData:
        clear();
        break;
    default:
        emit contentChanged();
        break;
    }
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

    auto types = modelManager()->eventTypes();
    if (id >= types.length())
        return result;

    QmlEventLocation location = types.at(id).location();

    result.insert(QStringLiteral("file"), location.filename());
    result.insert(QStringLiteral("line"), location.line());
    result.insert(QStringLiteral("column"), location.column());

    return result;
}

} // namespace QmlProfiler
