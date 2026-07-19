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

#include <qmldebug/qmlevent.h>
#include <qmldebug/qmleventtype.h>

#include <QHash>
#include <QStack>
#include <QPointer>
#include <QAbstractItemModel>

namespace Profiler::Internal {

class QmlProfilerModelManager;

class Quick3DFrameModel final : public QAbstractItemModel
{
public:
    struct Item {
        Item(int index = -1, int parent = -1, qint64 begin = 0, qint64 end = 0, int additionalType = 0, quint64 data = 0) :
            index(index), parent(parent), additionalType(additionalType), begin(begin), end(end), data(data) {}
        int index;
        int parent;
        int additionalType;
        qint64 begin;
        qint64 end;
        quint64 data;
        quint64 frameDelta = 0;
        int eventData = -1;
        QList<int> children;
        bool hasChildren() const
        {
            return !children.isEmpty();
        }
    };
    struct FrameTime
    {
        qint64 begin = 0;
        qint64 end  = 0;
        int frameCount = 0;
        bool inFrame = false;
        bool prepareReceived = false;
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
        TextureMemoryConsumption,
        FrameGroup,
        SubData
    };

    enum ColumnType
    {
        Frame,
        Duration,
        FrameDelta,
        Timestamp,
        View3D,
        MaxColumnType
    };

    enum ItemRole {
        SortRole = Qt::UserRole + 1, // Sort by data, not by displayed string
        FilterRole,
        CompareRole,
        IndexRole,
    };

    Quick3DFrameModel(QmlProfilerModelManager *manager, QObject *parent = nullptr);

    void clear();

    int rowCount(const QModelIndex &parent) const final;
    int columnCount(const QModelIndex &parent) const final;
    QVariant data(const QModelIndex &index, int role) const final;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const final;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const final;
    QModelIndex parent(const QModelIndex &index) const final;
    QString location(int index) const;

    QStringList view3DNames() const;
    QStringList frameNames(const QString &view3D) const;
    void setFilterView3D(const QString &view3D);
    void setFilterFrame(const QString &frame);

private:
    QList<int> frameIndices(const QString &view3DFilter) const;
    void loadEvent(const QmlDebug::QmlEvent &event, const QmlDebug::QmlEventType &type);
    const Item *findFrameGroup(const Item *item) const;
    void finalize();
    int parentRow(int index) const;

    bool m_oldEvents = false;
    QList<Item> m_data;
    Item m_stackBottom;
    QHash<int, FrameTime> m_frameTimes;
    QHash<int, int> m_eventData;
    QList<int> m_acceptedDetailTypes;
    QPointer<QmlProfilerModelManager> m_modelManager;
    int m_filterView3D = -1;
    int m_filterFrame = -1;
};

} // namespace Profiler::Internal
