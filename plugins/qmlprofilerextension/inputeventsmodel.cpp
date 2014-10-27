/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "inputeventsmodel.h"
#include "qmldebug/qmlprofilereventtypes.h"
#include "qmlprofiler/qmlprofilermodelmanager.h"

namespace QmlProfilerExtension {
namespace Internal {

using namespace QmlProfiler;

InputEventsModel::InputEventsModel(QObject *parent)
    : AbstractTimelineModel(tr(QmlProfilerModelManager::featureName(QmlDebug::ProfileInputEvents)),
                            QmlDebug::Event, QmlDebug::MaximumRangeType, parent)
{
}

quint64 InputEventsModel::features() const
{
    return 1 << QmlDebug::ProfileInputEvents;
}

int InputEventsModel::selectionId(int index) const
{
    return modelManager()->qmlModel()->getEventTypes()[typeId(index)].detailType;
}

QColor InputEventsModel::color(int index) const
{
    return colorBySelectionId(index);
}

QVariantList InputEventsModel::labels() const
{
    QVariantList result;

    if (expanded() && !hidden() && !isEmpty()) {
        {
            QVariantMap element;
            element.insert(QLatin1String("description"), QVariant(tr("Mouse Events")));
            element.insert(QLatin1String("id"), QVariant(QmlDebug::Mouse));
            result << element;
        }

        {
            QVariantMap element;
            element.insert(QLatin1String("description"), QVariant(tr("Keyboard Events")));
            element.insert(QLatin1String("id"), QVariant(QmlDebug::Key));
            result << element;
        }
    }

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

int InputEventsModel::row(int index) const
{
    if (!expanded())
        return 1;
    return selectionId(index) == QmlDebug::Mouse ? 1 : 2;
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
        insert(event.startTime, 0, event.typeIndex);
        updateProgress(count(), simpleModel->getEvents().count());
    }
    setCollapsedRowCount(2);
    setExpandedRowCount(3);
    updateProgress(1, 1);
}

bool InputEventsModel::accepted(const QmlProfilerDataModel::QmlEventTypeData &event) const
{
    return AbstractTimelineModel::accepted(event) &&
            (event.detailType == QmlDebug::Mouse || event.detailType == QmlDebug::Key);
}

}
}
