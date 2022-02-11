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
        quint64 data = 0;
        bool unload = false;
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

private:
    static QString messageType(uint i);
    static QString unloadMessageType(uint i);

    int m_maximumMsgType = 0;
    qint64 m_prevTexStartTime = -1;
    qint64 m_prevMeshStartTime = -1;
    quint64 m_prevMeshData = 0;
    quint64 m_prevTexData = 0;
    quint64 m_maxMeshSize = 0;
    quint64 m_maxTextureSize = 0;
    QVector<Item> m_data;
};

} // namespace Internal
} // namespace Qmlprofiler
