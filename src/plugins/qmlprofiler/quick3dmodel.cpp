// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmlprofilerconstants.h"
#include "qmlprofilertr.h"
#include "quick3dmodel.h"

#include <tracing/timelineformattime.h>

namespace QmlProfiler {
namespace Internal {

Quick3DModel::Quick3DModel(QmlProfilerModelManager *manager,
                                       Timeline::TimelineModelAggregator *parent) :
    QmlProfilerTimelineModel(manager, Quick3DEvent, UndefinedRangeType, ProfileQuick3D, parent),
    m_maximumMsgType(-1)
{
}

QRgb Quick3DModel::color(int index) const
{
    return colorBySelectionId(index);
}

static const char *messageTypes[] = {
    QT_TRANSLATE_NOOP("QmlProfiler", "Render Frame"),
    QT_TRANSLATE_NOOP("QmlProfiler", "Synchronize Frame"),
    QT_TRANSLATE_NOOP("QmlProfiler", "Prepare Frame"),
    QT_TRANSLATE_NOOP("QmlProfiler", "Mesh Load"),
    QT_TRANSLATE_NOOP("QmlProfiler", "Custom Mesh Load"),
    QT_TRANSLATE_NOOP("QmlProfiler", "Texture Load"),
    QT_TRANSLATE_NOOP("QmlProfiler", "Generate Shader"),
    QT_TRANSLATE_NOOP("QmlProfiler", "Load Shader"),
    QT_TRANSLATE_NOOP("QmlProfiler", "Particle Update"),

    QT_TRANSLATE_NOOP("QmlProfiler", "Mesh Memory consumption"),
    QT_TRANSLATE_NOOP("QmlProfiler", "Texture Memory consumption"),
};

static const char *unloadMessageTypes[] = {
    QT_TRANSLATE_NOOP("QmlProfiler", "Mesh Unload"),
    QT_TRANSLATE_NOOP("QmlProfiler", "Custom Mesh Unload"),
    QT_TRANSLATE_NOOP("QmlProfiler", "Texture Unload"),
};

QString Quick3DModel::messageType(uint i)
{
    return i < sizeof(messageTypes) / sizeof(char *) ? Tr::tr(messageTypes[i]) :
                                                       Tr::tr("Unknown Message %1").arg(i);
}

QString Quick3DModel::unloadMessageType(uint i)
{
    switch (i) {
    case MeshLoad:
        return Tr::tr(unloadMessageTypes[0]);
    case CustomMeshLoad:
        return Tr::tr(unloadMessageTypes[1]);
    case TextureLoad:
        return Tr::tr(unloadMessageTypes[2]);
    }
    return Tr::tr("Unknown Unload Message %1").arg(i);
}

QVariantList Quick3DModel::labels() const
{
    QVariantList result;
    for (int i = 0; i <= m_maximumMsgType; ++i) {
        QVariantMap element;
        element.insert(QLatin1String("displayName"),
                       i != ParticleUpdate ? Tr::tr("Render Thread") : Tr::tr("GUI Thread"));
        element.insert(QLatin1String("description"), messageType(i));
        element.insert(QLatin1String("id"), i);
        result << element;
    }
    return result;
}

float Quick3DModel::relativeHeight(int index) const
{
    auto detailType = m_data[index].additionalType;
    if (detailType == TextureMemoryConsumption)
        return qMin(1.0f, (float)m_data[index].data / (float)m_maxTextureSize);
    if (detailType == MeshMemoryConsumption)
        return qMin(1.0f, (float)m_data[index].data / (float)m_maxMeshSize);
    return 1.0f;
}

qint64 Quick3DModel::rowMaxValue(int rowNumber) const
{
    int index = rowNumber - 1;
    if (index == TextureMemoryConsumption)
        return m_maxTextureSize;
    if (index == MeshMemoryConsumption)
        return m_maxMeshSize;
    return 0;
}

QVariantMap Quick3DModel::details(int index) const
{
    auto detailType = m_data[index].additionalType;
    bool unload = m_data[index].unload;
    QVariantMap result;
    result.insert(QLatin1String("displayName"),
                  detailType != ParticleUpdate ? Tr::tr("Render Thread") : Tr::tr("GUI Thread"));
    result.insert(Tr::tr("Description"),
                  !unload ? messageType(detailType) : unloadMessageType(detailType));
    if (detailType < MeshMemoryConsumption)
        result.insert(Tr::tr("Duration"), Timeline::formatTime(duration(index)));
    if (detailType == ParticleUpdate)
        result.insert(Tr::tr("Count"), m_data[index].data);
    if (detailType == RenderFrame) {
        quint32 calls = m_data[index].data & 0xffffffff;
        quint32 passes = m_data[index].data >> 32;
        result.insert(Tr::tr("Draw Calls"), calls);
        result.insert(Tr::tr("Render Passes"), passes);
    }
    if ((detailType >= MeshLoad && detailType <= TextureLoad)
            || (detailType >= MeshMemoryConsumption && detailType <= TextureMemoryConsumption)) {
        result.insert(Tr::tr("Total Memory Usage"), m_data[index].data);
    }
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
    int detailType = type.detailType();
    if (detailType >= MaximumQuick3DFrameType)
        return;
    qint64 eventDuration = event.number<qint64>(0);
    qint64 eventTime = event.timestamp() - eventDuration;
    QVector<quint64> numbers = event.numbers<QVector<quint64>>();
    quint64 data = numbers.size() > 1 ? event.number<quint64>(1) : 0;

    int typeCount = detailType;
    if (detailType == MeshLoad || detailType == CustomMeshLoad) {
        bool updatePrevValues = true;
        if (m_prevMeshStartTime != -1) {
            bool unload = m_prevMeshData > data;
            m_data.insert(insert(eventTime, eventDuration, detailType),
                          Item(detailType, data, unload));
            if (m_prevMeshData != data) {
                m_data.insert(insert(m_prevMeshStartTime,
                                     eventTime - m_prevMeshStartTime, MeshMemoryConsumption),
                              Item(MeshMemoryConsumption, m_prevMeshData));
            } else {
                updatePrevValues = false;
            }
        } else {
            m_data.insert(insert(eventTime, eventDuration, detailType),
                          Item(detailType, data));
        }
        m_maxMeshSize = qMax(m_maxMeshSize, data);
        if (updatePrevValues) {
            m_prevMeshStartTime = eventTime;
            m_prevMeshData = data;
        }
        typeCount = MeshMemoryConsumption;
    } else if (detailType == TextureLoad) {
        bool updatePrevValues = true;
        if (m_prevTexStartTime != -1) {
            bool unload = m_prevTexData > data;
            m_data.insert(insert(eventTime, eventDuration, detailType),
                          Item(detailType, data, unload));
            if (m_prevTexData != data) {
                m_data.insert(insert(m_prevTexStartTime,
                                     eventTime - m_prevTexStartTime, TextureMemoryConsumption),
                              Item(TextureMemoryConsumption, m_prevTexData));
            } else {
                updatePrevValues = false;
            }
        } else {
            m_data.insert(insert(eventTime, eventDuration, detailType),
                          Item(detailType, data));
        }
        m_maxTextureSize = qMax(m_maxTextureSize, data);
        if (updatePrevValues) {
            m_prevTexData = data;
            m_prevTexStartTime = eventTime;
        }
        typeCount = TextureMemoryConsumption;
    } else {
        m_data.insert(insert(eventTime, eventDuration, detailType),
                      Item(detailType, data));
    }
    if (typeCount > m_maximumMsgType)
        m_maximumMsgType = typeCount;
}

void Quick3DModel::finalize()
{
    if (m_prevMeshStartTime != -1) {
        m_data.insert(insert(m_prevMeshStartTime, modelManager()->traceEnd() - m_prevMeshStartTime,
                             MeshMemoryConsumption),
                      Item(MeshMemoryConsumption, m_prevMeshData));
    }
    if (m_prevTexStartTime != -1) {
        m_data.insert(insert(m_prevTexStartTime, modelManager()->traceEnd() - m_prevTexStartTime,
                             TextureMemoryConsumption),
                      Item(TextureMemoryConsumption, m_prevTexData));
    }
    computeNesting();
    setCollapsedRowCount(Constants::QML_MIN_LEVEL + 1);
    setExpandedRowCount(m_maximumMsgType + 2);
    QmlProfilerTimelineModel::finalize();
}

void Quick3DModel::clear()
{
    m_data.clear();
    m_maximumMsgType = -1;
    m_prevTexStartTime = -1;
    m_prevMeshStartTime = -1;
    m_maxMeshSize = 0;
    m_maxTextureSize = 0;
    QmlProfilerTimelineModel::clear();
}

QVariantMap Quick3DModel::location(int index) const
{
    return locationFromTypeId(index);
}

} // namespace Internal
} // namespace QmlProfiler
