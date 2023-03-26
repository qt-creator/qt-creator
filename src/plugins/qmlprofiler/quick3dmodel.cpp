// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerconstants.h"
#include "qmlprofilertr.h"
#include "quick3dmodel.h"
#include <tracing/timelineformattime.h>

namespace QmlProfiler {
namespace Internal {

int Quick3DModel::eventDataId(int id)
{
    static int ID_MASK = 0xff000000;
    static int ID_MARKER = 0xed000000;
    if ((id & ID_MASK) == ID_MARKER)
        return id - ID_MARKER;
    return 0;
}

Quick3DModel::Quick3DModel(QmlProfilerModelManager *manager,
                           Timeline::TimelineModelAggregator *parent) :
    QmlProfilerTimelineModel(manager, Quick3DEvent, UndefinedRangeType, ProfileQuick3D, parent)
{
}

QRgb Quick3DModel::color(int index) const
{
    return colorBySelectionId(index);
}

static const char *messageTypes[] = {
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Render Frame"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Synchronize Frame"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Prepare Frame"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Mesh Load"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Custom Mesh Load"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Texture Load"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Generate Shader"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Load Shader"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Particle Update"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Render Call"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Render Pass"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Event Data"),

    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Mesh Memory consumption"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Texture Memory consumption"),
};

static const char *unloadMessageTypes[] = {
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Mesh Unload"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Custom Mesh Unload"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Texture Unload"),
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
    for (int detailType : m_sortedTypes) {
        QVariantMap element;
        element.insert(QLatin1String("displayName"),
                       detailType != ParticleUpdate ? Tr::tr("Render Thread") : Tr::tr("GUI Thread"));
        element.insert(QLatin1String("description"), messageType(detailType));
        element.insert(QLatin1String("id"), detailType);
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
    if (detailType == RenderPass)
        return qMin(1.0f, (float)m_data[index].nests / (float)m_maxNestedRenderCalls);
    return 1.0f;
}

qint64 Quick3DModel::rowMaxValue(int rowNumber) const
{
    int index = rowNumber - 1;
    if (index == TextureMemoryConsumption)
        return m_maxTextureSize;
    if (index == MeshMemoryConsumption)
        return m_maxMeshSize;
    if (index == RenderPass)
        return m_maxNestedRenderCalls;
    return 0;
}

bool Quick3DModel::resolveType(const QString &object, int detailType, QString &type)
{
    switch (detailType) {
    case RenderFrame:
    case SynchronizeFrame:
    case PrepareFrame:
        type = QLatin1String("View3D");
        break;
    case MeshLoad:
    case CustomMeshLoad:
    case TextureLoad:
    case LoadShader:
    case GenerateShader:
        type = QLatin1String("URL");
        break;
    case ParticleUpdate:
        type = QLatin1String("ParticleSystem");
        break;
    case RenderCall:
        if (object.contains(QLatin1String("Material")))
            type = QLatin1String("Material");
        if (object.contains(QLatin1String("Model")))
            type = QLatin1String("Model");
        return !type.isEmpty();
        break;
    case RenderPass:
        type = QLatin1String("Pass");
        break;
    default:
        break;
    }
    return !type.isEmpty();
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
    if ((detailType == RenderPass || detailType == PrepareFrame) && m_data[index].data) {
        quint32 width = m_data[index].data & 0xffffffff;
        quint32 height = m_data[index].data >> 32;
        result.insert(Tr::tr("Width"), width);
        result.insert(Tr::tr("Height"), height);
    }
    if ((detailType >= MeshLoad && detailType <= TextureLoad)
            || (detailType >= MeshMemoryConsumption && detailType <= TextureMemoryConsumption)) {
        result.insert(Tr::tr("Total Memory Usage"), m_data[index].data);
    }
    if (detailType == RenderCall) {
        quint32 primitives = m_data[index].data & 0xffffffff;
        quint32 instances = m_data[index].data >> 32;
        result.insert(Tr::tr("Primitives"), primitives);
        if (instances > 1)
            result.insert(Tr::tr("Instances"), instances);
    }
    if (!m_data[index].eventData.isEmpty()) {
        for (int i = 0; i < m_data[index].eventData.size(); i++) {
            int p = m_data[index].eventData[i];
            if (m_eventData.contains(p)) {
                const QmlEventType &et = modelManager()->eventType(m_eventData[p]);
                QString type;
                if (resolveType(et.data(), detailType, type))
                    result.insert(type, et.data());
            }
        }
    }
    return result;
}

int Quick3DModel::expandedRow(int index) const
{
    const Item &item = m_data[index];
    return m_sortedTypes.indexOf(item.additionalType) + 1;
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
    if (detailType == EventData) {
        m_eventData.insert(m_eventData.size() + 1, event.typeIndex());
        return;
    }

    QList<quint64> numbers = event.numbers<QList<quint64>>();
    if (numbers.size() == 0)
        return;
    qint64 eventDuration = numbers[0];
    qint64 eventTime = event.timestamp() - eventDuration;
    quint64 data = numbers.size() > 1 ? numbers[1] : 0;
    QList<int> eventData;
    // The rest are pairs of event data id's
    for (int i = 2; i < numbers.size(); i++) {
        qint32 h = eventDataId(numbers[i] >> 32);
        qint32 l = eventDataId(numbers[i] & 0xffffffff);
        if (m_eventData.contains(h))
            eventData.push_back(h);
        if (m_eventData.contains(l))
            eventData.push_back(l);
    }
    m_types << detailType;
    if (detailType == MeshLoad || detailType == CustomMeshLoad) {
        bool updatePrevValues = true;

        if (m_prevMeshStartTime != -1) {
            bool unload = m_prevMeshData > data;
            Item i = Item(detailType, data, unload);
            i.eventData = eventData;
            m_data.insert(insert(eventTime, eventDuration, detailType), i);
            if (m_prevMeshData != data) {
                m_data.insert(insert(m_prevMeshStartTime,
                                     eventTime - m_prevMeshStartTime, MeshMemoryConsumption),
                              Item(MeshMemoryConsumption, m_prevMeshData));
                m_types << MeshMemoryConsumption;
            } else {
                updatePrevValues = false;
            }
        } else {
            Item i = Item(detailType, data);
            i.eventData = eventData;
            m_data.insert(insert(eventTime, eventDuration, detailType), i);
        }
        m_maxMeshSize = qMax(m_maxMeshSize, data);
        if (updatePrevValues) {
            m_prevMeshStartTime = eventTime;
            m_prevMeshData = data;
        }
    } else if (detailType == TextureLoad) {
        bool updatePrevValues = true;

        if (m_prevTexStartTime != -1) {
            bool unload = m_prevTexData > data;
            Item i = Item(detailType, data, unload);
            i.eventData = eventData;
            m_data.insert(insert(eventTime, eventDuration, detailType), i);
            if (m_prevTexData != data) {
                m_data.insert(insert(m_prevTexStartTime,
                                     eventTime - m_prevTexStartTime, TextureMemoryConsumption),
                              Item(TextureMemoryConsumption, m_prevTexData));
                m_types << TextureMemoryConsumption;
            } else {
                updatePrevValues = false;
            }
        } else {
            Item i = Item(detailType, data, false);
            i.eventData = eventData;
            m_data.insert(insert(eventTime, eventDuration, detailType), i);
        }
        m_maxTextureSize = qMax(m_maxTextureSize, data);
        if (updatePrevValues) {
            m_prevTexData = data;
            m_prevTexStartTime = eventTime;
        }
    } else {
        Item i = Item(detailType, data);
        i.eventData = eventData;
        auto index = insert(eventTime, eventDuration, detailType);
        m_data.insert(index, i);
    }
}

void Quick3DModel::calculateRenderPassNesting()
{
    QList<qint64> nesting;
    for (int item = 0; item < m_data.size(); item++) {
        if (m_data[item].additionalType != RenderPass)
            continue;
        while (!nesting.isEmpty()) {
            auto l = nesting.last();
            if (l >= startTime(item))
                break;
            else
                nesting.removeLast();
        }
        nesting.push_back(endTime(item));
        m_data[item].nests = nesting.size();
        m_maxNestedRenderCalls = qMax(m_maxNestedRenderCalls, nesting.size());
    }
}

void Quick3DModel::finalize()
{
    if (m_prevMeshStartTime != -1) {
        m_data.insert(insert(m_prevMeshStartTime, modelManager()->traceEnd() - m_prevMeshStartTime,
                             MeshMemoryConsumption),
                      Item(MeshMemoryConsumption, m_prevMeshData));
        m_types << MeshMemoryConsumption;
    }
    if (m_prevTexStartTime != -1) {
        m_data.insert(insert(m_prevTexStartTime, modelManager()->traceEnd() - m_prevTexStartTime,
                             TextureMemoryConsumption),
                      Item(TextureMemoryConsumption, m_prevTexData));
        m_types << TextureMemoryConsumption;
    }
    computeNesting();
    setCollapsedRowCount(Constants::QML_MIN_LEVEL + 1);
    m_sortedTypes = m_types.values();
    std::sort(m_sortedTypes.begin(), m_sortedTypes.end(), [](int a, int b){ return a < b;});
    setExpandedRowCount(m_sortedTypes.size() + 1);
    QmlProfilerTimelineModel::finalize();
    calculateRenderPassNesting();

}

void Quick3DModel::clear()
{
    m_data.clear();
    m_types.clear();
    m_sortedTypes.clear();
    m_prevTexStartTime = -1;
    m_prevMeshStartTime = -1;
    m_maxMeshSize = 0;
    m_maxTextureSize = 0;
    QmlProfilerTimelineModel::clear();
}

QVariantMap Quick3DModel::locationFromEvent(int index) const
{
    QVariantMap ret;
    for (auto e : m_data[index].eventData) {
        if (m_eventData.contains(e)) {
            const QmlEventType &et = modelManager()->eventType(m_eventData[e]);
            const QString data = et.data();
            QString file, line;
            int lineIdx = data.lastIndexOf(QStringLiteral(".qml:"));
            int nameIdx = data.lastIndexOf(QStringLiteral(" "));
            if (lineIdx < 0)
                return ret;
            lineIdx += 4;
            file = data.mid(nameIdx + 1, lineIdx - nameIdx - 1);
            line = data.right(data.length() - lineIdx - 1);
            QUrl url(file);
            ret.insert(QStringLiteral("file"), url.fileName());
            ret.insert(QStringLiteral("line"), line.toInt());
            ret.insert(QStringLiteral("column"), 0);
            break;
        }
    }
    return ret;
}

QVariantMap Quick3DModel::location(int index) const
{
    QVariantMap ret;
    if (!m_data[index].eventData.isEmpty())
        ret = locationFromEvent(index);
    if (!ret.isEmpty())
        return ret;
    return locationFromTypeId(index);
}

int Quick3DModel::typeId(int index) const
{
    for (auto ed : m_data[index].eventData) {
        if (m_eventData.contains(ed))
            return m_eventData[ed];
    }
    return QmlProfilerTimelineModel::typeId(index);
}

} // namespace Internal
} // namespace QmlProfiler
