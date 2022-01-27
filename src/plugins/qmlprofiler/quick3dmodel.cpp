/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "quick3dmodel.h"
#include "qmlprofilerconstants.h"
#include <tracing/timelineformattime.h>


namespace QmlProfiler {
namespace Internal {

Quick3DModel::Quick3DModel(QmlProfilerModelManager *manager,
                                       Timeline::TimelineModelAggregator *parent) :
    QmlProfilerTimelineModel(manager, Quick3DEvent, MaximumRangeType, ProfileQuick3D, parent),
    m_maximumMsgType(-1)
{
}

int Quick3DModel::typeId(int index) const
{
    return m_data[index].typeId;
}

QRgb Quick3DModel::color(int index) const
{
    return colorBySelectionId(index);
}

static const char *messageTypes[] = {
    QT_TRANSLATE_NOOP("Quick3DModel", "Render Frame"),
    QT_TRANSLATE_NOOP("Quick3DModel", "Synchronize Frame"),
    QT_TRANSLATE_NOOP("Quick3DModel", "Prepare Frame"),
    QT_TRANSLATE_NOOP("Quick3DModel", "Mesh Load"),
    QT_TRANSLATE_NOOP("Quick3DModel", "Custom Mesh Load"),
    QT_TRANSLATE_NOOP("Quick3DModel", "Texture Load"),
    QT_TRANSLATE_NOOP("Quick3DModel", "Generate Shader"),
    QT_TRANSLATE_NOOP("Quick3DModel", "Load Shader"),
    QT_TRANSLATE_NOOP("Quick3DModel", "Particle Update"),
};

QString Quick3DModel::messageType(uint i)
{
    return i < sizeof(messageTypes) / sizeof(char *) ? tr(messageTypes[i]) :
                                                       tr("Unknown Message %1").arg(i);
}

QVariantList Quick3DModel::labels() const
{
    QVariantList result;
    for (int i = 0; i <= m_maximumMsgType; ++i) {
        QVariantMap element;
        element.insert(QLatin1String("displayName"), i < ParticleUpdate ? tr("Render Thread") : tr("GUI Thread"));
        element.insert(QLatin1String("description"), messageType(i));
        element.insert(QLatin1String("id"), i);
        result << element;
    }
    return result;
}

QVariantMap Quick3DModel::details(int index) const
{
    const QmlProfilerModelManager *manager = modelManager();
    const QmlEventType &type = manager->eventType(m_data[index].typeId);

    QVariantMap result;
    result.insert(QLatin1String("displayName"), type.detailType() < ParticleUpdate ? tr("Render Thread") : tr("GUI Thread"));
    result.insert(tr("Description"), messageType(type.detailType()));
    result.insert(tr("Duration"), Timeline::formatTime(duration(index)));
    if (type.detailType() == ParticleUpdate)
        result.insert(tr("Count"), m_data[index].data);
    return result;
}

int Quick3DModel::expandedRow(int index) const
{
    return selectionId(index) + 1;
}

int Quick3DModel::collapsedRow(int index) const
{
    Q_UNUSED(index)
    return Constants::QML_MIN_LEVEL;
}

void Quick3DModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    qint64 eventDuration = event.number<qint64>(0);
    qint64 startTime = event.timestamp() - eventDuration;
    if (type.detailType() == MessageType::ParticleUpdate) {
        quint64 particleCount = event.number<qint64>(1);
        m_data.insert(insert(startTime, eventDuration, type.detailType()),
                      Item(event.typeIndex(), particleCount));
    } else {
        m_data.insert(insert(startTime, eventDuration, type.detailType()),
                      Item(event.typeIndex(), 0));
    }

    if (type.detailType() > m_maximumMsgType)
        m_maximumMsgType = type.detailType();
}

void Quick3DModel::finalize()
{
    setCollapsedRowCount(Constants::QML_MIN_LEVEL + 1);
    setExpandedRowCount(m_maximumMsgType + 2);
    QmlProfilerTimelineModel::finalize();
}

void Quick3DModel::clear()
{
    m_data.clear();
    m_maximumMsgType = -1;
    QmlProfilerTimelineModel::clear();
}

QVariantMap Quick3DModel::location(int index) const
{
    return locationFromTypeId(index);
}

} // namespace Internal
} // namespace QmlProfiler
