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

#include "inputeventsmodel.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilereventtypes.h"

#include <timeline/timelineformattime.h>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QMetaEnum>

namespace QmlProfiler {
namespace Internal {

InputEventsModel::InputEventsModel(QmlProfilerModelManager *manager, QObject *parent) :
    QmlProfilerTimelineModel(manager, Event, MaximumRangeType, ProfileInputEvents, parent),
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
    element.insert(QLatin1String("description"), QVariant(tr("Mouse Events")));
    element.insert(QLatin1String("id"), QVariant(Mouse));
    result << element;

    element.clear();
    element.insert(QLatin1String("description"), QVariant(tr("Keyboard Events")));
    element.insert(QLatin1String("id"), QVariant(Key));
    result << element;

    return result;
}

QMetaEnum InputEventsModel::metaEnum(const char *name)
{
    return staticQtMetaObject.enumerator(staticQtMetaObject.indexOfEnumerator(name));
}

QVariantMap InputEventsModel::details(int index) const
{
    QVariantMap result;
    result.insert(tr("Timestamp"), Timeline::formatTime(startTime(index),
                                                        modelManager()->traceTime()->duration()));
    QString type;
    const InputEvent &event = m_data[index];
    switch (event.type) {
    case InputKeyPress:
        type = tr("Key Press");
        // fallthrough
    case InputKeyRelease:
        if (type.isEmpty())
            type = tr("Key Release");
        if (event.a != 0) {
            result.insert(tr("Key"), QLatin1String(metaEnum("Key").valueToKey(event.a)));
        }
        if (event.b != 0) {
            result.insert(tr("Modifiers"),
                          QLatin1String(metaEnum("KeyboardModifiers").valueToKeys(event.b)));
        }
        break;
    case InputMouseDoubleClick:
        type = tr("Double Click");
        // fallthrough
    case InputMousePress:
        if (type.isEmpty())
            type = tr("Mouse Press");
        // fallthrough
    case InputMouseRelease:
        if (type.isEmpty())
            type = tr("Mouse Release");
        result.insert(tr("Button"), QLatin1String(metaEnum("MouseButtons").valueToKey(event.a)));
        result.insert(tr("Result"), QLatin1String(metaEnum("MouseButtons").valueToKeys(event.b)));
        break;
    case InputMouseMove:
        type = tr("Mouse Move");
        result.insert(tr("X"), QString::number(event.a));
        result.insert(tr("Y"), QString::number(event.b));
        break;
    case InputMouseWheel:
        type = tr("Mouse Wheel");
        result.insert(tr("Angle X"), QString::number(event.a));
        result.insert(tr("Angle Y"), QString::number(event.b));
        break;
    case InputKeyUnknown:
        type = tr("Keyboard Event");
        break;
    case InputMouseUnknown:
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
    return selectionId(index) == Mouse ? 1 : 2;
}

int InputEventsModel::collapsedRow(int index) const
{
    Q_UNUSED(index)
    return 1;
}

void InputEventsModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    m_data.insert(insert(event.timestamp(), 0, type.detailType()),
                  InputEvent(static_cast<InputEventType>(event.number<qint32>(0)),
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
}

void InputEventsModel::clear()
{
    m_keyTypeId = m_mouseTypeId = -1;
    m_data.clear();
    QmlProfilerTimelineModel::clear();
}

bool InputEventsModel::accepted(const QmlEventType &type) const
{
    return QmlProfilerTimelineModel::accepted(type) &&
            (type.detailType() == Mouse || type.detailType() == Key);
}

InputEventsModel::InputEvent::InputEvent(InputEventType type, int a, int b) :
    type(type), a(a), b(b)
{
}

} // namespace Internal
} // namespace QmlProfiler
