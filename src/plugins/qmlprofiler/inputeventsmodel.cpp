// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "inputeventsmodel.h"
#include "qmlprofilereventtypes.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilertr.h"

#include <tracing/timelineformattime.h>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QMetaEnum>

namespace QmlProfiler {
namespace Internal {

InputEventsModel::InputEventsModel(QmlProfilerModelManager *manager,
                                   Timeline::TimelineModelAggregator *parent) :
    QmlProfilerTimelineModel(manager, Event, UndefinedRangeType, ProfileInputEvents, parent),
    m_keyTypeId(-1), m_mouseTypeId(-1)
{
}

int InputEventsModel::typeId(int index) const
{
    return selectionId(index) == Mouse ? m_mouseTypeId : m_keyTypeId;
}

QRgb InputEventsModel::color(int index) const
{
    return colorBySelectionId(index);
}

QVariantList InputEventsModel::labels() const
{
    QVariantList result;

    QVariantMap element;
    element.insert(QLatin1String("description"), QVariant(Tr::tr("Mouse Events")));
    element.insert(QLatin1String("id"), QVariant(Mouse));
    result << element;

    element.clear();
    element.insert(QLatin1String("description"), QVariant(Tr::tr("Keyboard Events")));
    element.insert(QLatin1String("id"), QVariant(Key));
    result << element;

    return result;
}

QMetaEnum InputEventsModel::metaEnum(const char *name)
{
    return Qt::staticMetaObject.enumerator(Qt::staticMetaObject.indexOfEnumerator(name));
}

QVariantMap InputEventsModel::details(int index) const
{
    QVariantMap result;
    result.insert(Tr::tr("Timestamp"), Timeline::formatTime(startTime(index),
                                                        modelManager()->traceDuration()));
    QString type;
    const Item &event = m_data[index];
    switch (event.type) {
    case InputKeyPress:
        type = Tr::tr("Key Press");
        Q_FALLTHROUGH();
    case InputKeyRelease:
        if (type.isEmpty())
            type = Tr::tr("Key Release");
        if (event.a != 0) {
            result.insert(Tr::tr("Key"), QLatin1String(metaEnum("Key").valueToKey(event.a)));
        }
        if (event.b != 0) {
            result.insert(Tr::tr("Modifiers"),
                          QLatin1String(metaEnum("KeyboardModifiers").valueToKeys(event.b)));
        }
        break;
    case InputMouseDoubleClick:
        type = Tr::tr("Double Click");
        Q_FALLTHROUGH();
    case InputMousePress:
        if (type.isEmpty())
            type = Tr::tr("Mouse Press");
        Q_FALLTHROUGH();
    case InputMouseRelease:
        if (type.isEmpty())
            type = Tr::tr("Mouse Release");
        result.insert(Tr::tr("Button"), QLatin1String(metaEnum("MouseButtons").valueToKey(event.a)));
        result.insert(Tr::tr("Result"), QLatin1String(metaEnum("MouseButtons").valueToKeys(event.b)));
        break;
    case InputMouseMove:
        type = Tr::tr("Mouse Move");
        result.insert(Tr::tr("X"), QString::number(event.a));
        result.insert(Tr::tr("Y"), QString::number(event.b));
        break;
    case InputMouseWheel:
        type = Tr::tr("Mouse Wheel");
        result.insert(Tr::tr("Angle X"), QString::number(event.a));
        result.insert(Tr::tr("Angle Y"), QString::number(event.b));
        break;
    case InputKeyUnknown:
        type = Tr::tr("Keyboard Event");
        break;
    case InputMouseUnknown:
        type = Tr::tr("Mouse Event");
        break;
    default:
        type = Tr::tr("Unknown");
        break;
    }

    result.insert(QLatin1String("displayName"), type);

    return result;
}

int InputEventsModel::expandedRow(int index) const
{
    return selectionId(index) == Mouse ? 1 : 2;
}

int InputEventsModel::collapsedRow(int index) const
{
    Q_UNUSED(index)
    return 1;
}

void InputEventsModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    if (type.detailType() >= MaximumInputEventType)
        return;
    m_data.insert(insert(event.timestamp(), 0, type.detailType()),
                  Item(static_cast<InputEventType>(event.number<qint32>(0)),
                             event.number<qint32>(1), event.number<qint32>(2)));

    if (type.detailType() == Mouse) {
        if (m_mouseTypeId == -1)
            m_mouseTypeId = event.typeIndex();
    } else if (m_keyTypeId == -1) {
        m_keyTypeId = event.typeIndex();
    }
}

void InputEventsModel::finalize()
{
    setCollapsedRowCount(2);
    setExpandedRowCount(3);
    QmlProfilerTimelineModel::finalize();
}

void InputEventsModel::clear()
{
    m_keyTypeId = m_mouseTypeId = -1;
    m_data.clear();
    QmlProfilerTimelineModel::clear();
}

InputEventsModel::Item::Item(InputEventType type, int a, int b) :
    type(type), a(a), b(b)
{
}

} // namespace Internal
} // namespace QmlProfiler
