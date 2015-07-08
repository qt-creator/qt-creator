/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlprofilertimelinemodel.h"

namespace QmlProfiler {

QmlProfilerTimelineModel::QmlProfilerTimelineModel(QmlProfilerModelManager *modelManager,
                                                   QmlDebug::Message message,
                                                   QmlDebug::RangeType rangeType,
                                                   QmlDebug::ProfileFeature mainFeature,
                                                   QObject *parent) :
    TimelineModel(modelManager->registerModelProxy(),
                  tr(QmlProfilerModelManager::featureName(mainFeature)), parent),
    m_message(message), m_rangeType(rangeType), m_mainFeature(mainFeature),
    m_modelManager(modelManager)
{
    connect(modelManager, &QmlProfilerModelManager::stateChanged,
            this, &QmlProfilerTimelineModel::dataChanged);
    connect(modelManager, &QmlProfilerModelManager::visibleFeaturesChanged,
            this, &QmlProfilerTimelineModel::onVisibleFeaturesChanged);
    announceFeatures(1ULL << m_mainFeature);
}

QmlDebug::RangeType QmlProfilerTimelineModel::rangeType() const
{
    return m_rangeType;
}

QmlDebug::Message QmlProfilerTimelineModel::message() const
{
    return m_message;
}

QmlDebug::ProfileFeature QmlProfilerTimelineModel::mainFeature() const
{
    return m_mainFeature;
}

bool QmlProfilerTimelineModel::accepted(const QmlProfilerDataModel::QmlEventTypeData &event) const
{
    return (event.rangeType == m_rangeType && event.message == m_message);
}

bool QmlProfilerTimelineModel::handlesTypeId(int typeIndex) const
{
    if (typeIndex < 0)
        return false;

    return accepted(modelManager()->qmlModel()->getEventTypes().at(typeIndex));
}

void QmlProfilerTimelineModel::clear()
{
    TimelineModel::clear();
    updateProgress(0, 1);
}

QmlProfilerModelManager *QmlProfilerTimelineModel::modelManager() const
{
    return m_modelManager;
}

void QmlProfilerTimelineModel::updateProgress(qint64 count, qint64 max) const
{
    m_modelManager->modelProxyCountUpdated(modelId(), count, max);
}

void QmlProfilerTimelineModel::announceFeatures(quint64 features) const
{
    m_modelManager->announceFeatures(modelId(), features);
}

void QmlProfilerTimelineModel::dataChanged()
{

    switch (m_modelManager->state()) {
    case QmlProfilerDataState::Done:
        loadData();
        emit emptyChanged();
        break;
    case QmlProfilerDataState::ClearingData:
        clear();
        break;
    default:
        break;
    }

    emit labelsChanged();
}

void QmlProfilerTimelineModel::onVisibleFeaturesChanged(quint64 features)
{
    setHidden(!(features & (1ULL << m_mainFeature)));
}

int QmlProfilerTimelineModel::bindingLoopDest(int index) const
{
    Q_UNUSED(index);
    return -1;
}

}
