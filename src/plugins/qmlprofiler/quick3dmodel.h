// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilertimelinemodel.h"

namespace QmlProfiler {
namespace Internal {

class Quick3DModel : public QmlProfilerTimelineModel
{
    Q_OBJECT

public:
    struct Item {
        Item(int additionalType, quint64 data, bool unload = false) :
            additionalType(additionalType), data(data), unload(unload) {}
        int additionalType = 0;
        int nests = 0;
        quint64 data = 0;
        bool unload = false;
        QList<int> eventData;
    };

    enum MessageType
    {
        RenderFrame,
        SynchronizeFrame,
        PrepareFrame,
        MeshLoad,
        CustomMeshLoad,
        TextureLoad,
        GenerateShader,
        LoadShader,
        ParticleUpdate,
        RenderCall,
        RenderPass,
        EventData,
        // additional types
        MeshMemoryConsumption,
        TextureMemoryConsumption
    };

    Quick3DModel(QmlProfilerModelManager *manager, Timeline::TimelineModelAggregator *parent);

    QRgb color(int index) const override;
    QVariantList labels() const override;
    QVariantMap details(int index) const override;
    int expandedRow(int index) const override;
    int collapsedRow(int index) const override;
    qint64 rowMaxValue(int rowNumber) const override;
    float relativeHeight(int index) const override;
    void loadEvent(const QmlEvent &event, const QmlEventType &type) override;
    void finalize() override;
    void clear() override;
    QVariantMap location(int index) const override;
    int typeId(int index) const override;

    static int eventDataId(int id);

private:
    static QString messageType(uint i);
    static QString unloadMessageType(uint i);
    static bool resolveType(const QString &object, int detailType, QString &type);
    QVariantMap locationFromEvent(int poid) const;
    void calculateRenderPassNesting();

    QSet<int> m_types;
    QList<int> m_sortedTypes;
    qint64 m_prevTexStartTime = -1;
    qint64 m_prevMeshStartTime = -1;
    quint64 m_prevMeshData = 0;
    quint64 m_prevTexData = 0;
    quint64 m_maxMeshSize = 0;
    quint64 m_maxTextureSize = 0;
    int m_maxNestedRenderCalls = 1;
    QVector<Item> m_data;
    QHash<int, int> m_eventData;
};

} // namespace Internal
} // namespace Qmlprofiler
