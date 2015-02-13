/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "inputeventsmodel.h"
#include "qmldebug/qmlprofilereventtypes.h"
#include "qmlprofiler/qmlprofilermodelmanager.h"

namespace QmlProfilerExtension {
namespace Internal {

using namespace QmlProfiler;

InputEventsModel::InputEventsModel(QmlProfilerModelManager *manager, QObject *parent) :
    QmlProfilerTimelineModel(manager,
                             tr(QmlProfilerModelManager::featureName(QmlDebug::ProfileInputEvents)),
                             QmlDebug::Event, QmlDebug::MaximumRangeType, parent),
      m_keyTypeId(-1), m_mouseTypeId(-1)
{
    announceFeatures(1 << QmlDebug::ProfileInputEvents);
}

int InputEventsModel::typeId(int index) const
{
    return selectionId(index) == QmlDebug::Mouse ? m_mouseTypeId : m_keyTypeId;
}

QColor InputEventsModel::color(int index) const
{
    return colorBySelectionId(index);
}

QVariantList InputEventsModel::labels() const
{
    QVariantList result;

    QVariantMap element;
    element.insert(QLatin1String("description"), QVariant(tr("Mouse Events")));
    element.insert(QLatin1String("id"), QVariant(QmlDebug::Mouse));
    result << element;

    element.clear();
    element.insert(QLatin1String("description"), QVariant(tr("Keyboard Events")));
    element.insert(QLatin1String("id"), QVariant(QmlDebug::Key));
    result << element;

    return result;
}

QVariantMap InputEventsModel::details(int index) const
{
    QVariantMap result;
    result.insert(QLatin1String("displayName"),
                  selectionId(index) == QmlDebug::Key ? tr("Keyboard Event") : tr("Mouse Event"));
    result.insert(QLatin1String("Timestamp"), QmlProfilerBaseModel::formatTime(startTime(index)));
    return result;
}

int InputEventsModel::expandedRow(int index) const
{
    return selectionId(index) == QmlDebug::Mouse ? 1 : 2;
}

int InputEventsModel::collapsedRow(int index) const
{
    Q_UNUSED(index)
    return 1;
}

void InputEventsModel::loadData()
{
    clear();
    QmlProfilerDataModel *simpleModel = modelManager()->qmlModel();
    if (simpleModel->isEmpty())
        return;

    const QVector<QmlProfilerDataModel::QmlEventTypeData> &types = simpleModel->getEventTypes();
    foreach (const QmlProfilerDataModel::QmlEventData &event, simpleModel->getEvents()) {
        const QmlProfilerDataModel::QmlEventTypeData &type = types[event.typeIndex];
        if (!accepted(type))
            continue;
        insert(event.startTime, 0, type.detailType);
        if (type.detailType == QmlDebug::Mouse) {
            if (m_mouseTypeId == -1)
                m_mouseTypeId = event.typeIndex;
        } else if (m_keyTypeId == -1) {
            m_keyTypeId = event.typeIndex;
        }
        updateProgress(count(), simpleModel->getEvents().count());
    }
    setCollapsedRowCount(2);
    setExpandedRowCount(3);
    updateProgress(1, 1);
}

void InputEventsModel::clear()
{
    m_keyTypeId = m_mouseTypeId = -1;
    QmlProfilerTimelineModel::clear();
}

bool InputEventsModel::accepted(const QmlProfilerDataModel::QmlEventTypeData &event) const
{
    return QmlProfilerTimelineModel::accepted(event) &&
            (event.detailType == QmlDebug::Mouse || event.detailType == QmlDebug::Key);
}

}
}
