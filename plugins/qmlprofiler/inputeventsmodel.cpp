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

#include <QKeyEvent>
#include <QMouseEvent>
#include <QMetaEnum>

namespace QmlProfilerExtension {
namespace Internal {

using namespace QmlProfiler;

InputEventsModel::InputEventsModel(QmlProfilerModelManager *manager, QObject *parent) :
    QmlProfilerTimelineModel(manager, QmlDebug::Event, QmlDebug::MaximumRangeType,
                             QmlDebug::ProfileInputEvents, parent),
    m_keyTypeId(-1), m_mouseTypeId(-1)
{
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
    result.insert(tr("Timestamp"), QmlProfilerDataModel::formatTime(startTime(index)));
    QString type;
    const InputEvent &event = m_data[index];
    switch (event.type) {
    case QmlDebug::InputKeyPress:
        type = tr("Key Press");
    case QmlDebug::InputKeyRelease:
        if (type.isEmpty())
            type = tr("Key Release");
        if (event.a != 0) {
            result.insert(tr("Key"), QLatin1String(
                              QMetaEnum::fromType<Qt::Key>().valueToKey(event.a)));
        }
        if (event.b != 0) {
            result.insert(tr("Modifiers"), QLatin1String(
                              QMetaEnum::fromType<Qt::KeyboardModifiers>().valueToKeys(event.b)));
        }
        break;
    case QmlDebug::InputMouseDoubleClick:
        type = tr("Double Click");
    case QmlDebug::InputMousePress:
        if (type.isEmpty())
            type = tr("Mouse Press");
    case QmlDebug::InputMouseRelease:
        if (type.isEmpty())
            type = tr("Mouse Release");
        result.insert(tr("Button"), QLatin1String(
                          QMetaEnum::fromType<Qt::MouseButtons>().valueToKey(event.a)));
        result.insert(tr("Result"), QLatin1String(
                          QMetaEnum::fromType<Qt::MouseButtons>().valueToKeys(event.b)));
        break;
    case QmlDebug::InputMouseMove:
        type = tr("Mouse Move");
        result.insert(tr("X"), QString::number(event.a));
        result.insert(tr("Y"), QString::number(event.b));
        break;
    case QmlDebug::InputMouseWheel:
        type = tr("Mouse Wheel");
        result.insert(tr("Angle X"), QString::number(event.a));
        result.insert(tr("Angle Y"), QString::number(event.b));
        break;
    case QmlDebug::InputKeyUnknown:
        type = tr("Keyboard Event");
        break;
    case QmlDebug::InputMouseUnknown:
        type = tr("Mouse Event");
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    result.insert(QLatin1String("displayName"), type);

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
    QmlProfilerDataModel *simpleModel = modelManager()->qmlModel();
    if (simpleModel->isEmpty())
        return;

    const QVector<QmlProfilerDataModel::QmlEventTypeData> &types = simpleModel->getEventTypes();
    foreach (const QmlProfilerDataModel::QmlEventData &event, simpleModel->getEvents()) {
        const QmlProfilerDataModel::QmlEventTypeData &type = types[event.typeIndex()];
        if (!accepted(type))
            continue;

        m_data.insert(insert(event.startTime(), 0, type.detailType),
                      InputEvent(static_cast<QmlDebug::InputEventType>(event.numericData(0)),
                                 event.numericData(1), event.numericData(2)));

        if (type.detailType == QmlDebug::Mouse) {
            if (m_mouseTypeId == -1)
                m_mouseTypeId = event.typeIndex();
        } else if (m_keyTypeId == -1) {
            m_keyTypeId = event.typeIndex();
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
    m_data.clear();
    QmlProfilerTimelineModel::clear();
}

bool InputEventsModel::accepted(const QmlProfilerDataModel::QmlEventTypeData &event) const
{
    return QmlProfilerTimelineModel::accepted(event) &&
            (event.detailType == QmlDebug::Mouse || event.detailType == QmlDebug::Key);
}

InputEventsModel::InputEvent::InputEvent(QmlDebug::InputEventType type, int a, int b) :
    type(type), a(a), b(b)
{
}

}
}
