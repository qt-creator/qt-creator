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

#include "quick3dframemodel.h"

#include "profilertr.h"
#include "qmlprofilermodelmanager.h"
#include "quick3dmodel.h"

#include <tracing/timelineformattime.h>

using namespace QmlDebug;

namespace Profiler::Internal {

Quick3DFrameModel::Quick3DFrameModel(QmlProfilerModelManager *modelManager, QObject *parent)
    : QAbstractItemModel(parent)
    , m_modelManager(modelManager)
{
    m_acceptedDetailTypes << RenderFrame << SynchronizeFrame << PrepareFrame << RenderCall << RenderPass << EventData << TextureLoad << MeshLoad << CustomMeshLoad;
    modelManager->qmlLoaders.append({1ULL << ProfileQuick3D,
                                     std::bind(&Quick3DFrameModel::loadEvent, this,
                                               std::placeholders::_1, std::placeholders::_2)});
    modelManager->registerFeatures(1ULL << ProfileQuick3D,
                                   std::bind(&Quick3DFrameModel::beginResetModel, this),
                                   std::bind(&Quick3DFrameModel::finalize, this),
                                   std::bind(&Quick3DFrameModel::clear, this));
}

void Quick3DFrameModel::clear()
{
    beginResetModel();
    m_data.clear();
    m_stackBottom = {};
    m_frameTimes.clear();
    m_eventData.clear();
    m_oldEvents = false;
    endResetModel();
}

int Quick3DFrameModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_stackBottom.children.size();
    int index = parent.internalId();
    if (index >= 0) {
        const Item &i = m_data[index];
        return i.children.size();
    } else {
        return m_stackBottom.children.size();
    }
}

int Quick3DFrameModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return MaxColumnType;
}

QVariant Quick3DFrameModel::data(const QModelIndex &index, int role) const
{
    const Item &item = m_data[index.internalId()];
    QString strData;
    if (m_eventData.contains(item.eventData))
        strData = m_modelManager->eventType(m_eventData[item.eventData]).data();
    QVariant result;
    switch (role) {
    case Qt::DisplayRole: {
        switch (index.column()) {
        case Frame: {
            switch (item.additionalType) {
            case SynchronizeFrame: {
                    QString data = "Synchronize Frame: ";
                    if (item.data) {
                        quint32 w = item.data & 0xffffffff;
                        quint32 h = item.data >> 32;
                        data += ", Render target size: " + QString::number(w) + "x" + QString::number(h);
                    }
                    result = QVariant::fromValue(data);
                } break;
            case RenderFrame: {
                    QString data = "Render Frame: ";
                    if (item.data) {
                        quint32 calls = item.data & 0xffffffff;
                        quint32 passes = item.data >> 32;
                        data += "Render Calls: " + QString::number(calls) + ", Render Passes: " + QString::number(passes);
                    }
                    result = QVariant::fromValue(data);
                } break;
            case PrepareFrame: {
                    QString data = "Prepare Frame: ";
                    if (item.data) {
                        quint32 w = item.data & 0xffffffff;
                        quint32 h = item.data >> 32;
                        data += ", Render target size: " + QString::number(w) + "x" + QString::number(h);
                    }
                    result = QVariant::fromValue(data);
                } break;
            case RenderCall: {
                    QString data = "Render Call: ";
                    if (item.data) {
                        quint32 primitives = item.data & 0xffffffff;
                        quint32 instances = item.data >> 32;
                        data += "Primitives: " + QString::number(primitives) + ", Instances: " + QString::number(instances);
                    }
                    result = QVariant::fromValue(data);
                } break;
            case RenderPass: {
                    QString data = "Render Pass: " + strData;
                    if (item.data) {
                        quint32 w = item.data & 0xffffffff;
                        quint32 h = item.data >> 32;
                        data += ", Render target size: " + QString::number(w) + "x" + QString::number(h);
                    }
                    result = QVariant::fromValue(data);
                } break;
            case FrameGroup: {
                    QString data = "Frame " + QString::number(item.data);
                    result = QVariant::fromValue(data);
                } break;
            case TextureLoad: {
                    QString data = "Texture Load " + strData;
                    result = QVariant::fromValue(data);
                } break;
            case MeshLoad: {
                    QString data = "Mesh Load " + strData;
                    result = QVariant::fromValue(data);
                } break;
            case CustomMeshLoad: {
                    QString data = "Custom Mesh Load " + strData;
                    result = QVariant::fromValue(data);
                } break;
            case SubData: {
                if (strData.contains(QLatin1String("Material")))
                    strData = QLatin1String("Material: ") + strData;
                else if (strData.contains(QLatin1String("Model")))
                    strData = QLatin1String("Model: ") + strData;
                else if (strData.contains(QLatin1String("Particle")))
                    strData = QLatin1String("Particle: ") + strData;
                else
                    strData = QLatin1String("Object: ") + strData;
                result = QVariant::fromValue(strData);
                } break;
            }
        } break;
        case Timestamp: {
            QString data = Timeline::formatTime(item.begin);
            result = QVariant::fromValue(data);
        } break;
        case Duration: {
            QString data = Timeline::formatTime(item.end - item.begin);
            result = QVariant::fromValue(data);
        } break;
        case FrameDelta: {
            if (item.frameDelta) {
                QString data = Timeline::formatTime(item.frameDelta);
                result = QVariant::fromValue(data);
            }
        } break;
        case View3D: {
            // Don't show view3d object anywhere but FrameGroup
            const bool isView3D = m_frameTimes.contains(item.eventData);
            if (isView3D && item.additionalType == FrameGroup)
                result = QVariant::fromValue(strData);
        } break;
        }
    } break;
    case SortRole: {
        switch (index.column()) {
        case Frame:
            if (item.additionalType == FrameGroup)
                result = QVariant::fromValue(item.data);
            else
                result = QVariant::fromValue(item.index);
            break;
        case Timestamp:
            result = QVariant::fromValue(item.begin);
            break;
        case Duration:
            result = QVariant::fromValue(item.end - item.begin);
            break;
        case FrameDelta:
            if (item.frameDelta)
                result = QVariant::fromValue(item.frameDelta);
            break;
        default:
            break;
        }
    } break;
    case IndexRole: {
        result = QVariant::fromValue(item.index);
        } break;
    case FilterRole: {
        const Item *group = findFrameGroup(&item);
        if (group)
            result = QVariant::fromValue(m_modelManager->eventType(m_eventData[group->eventData]).data());
        } break;
    case CompareRole: {
        const Item *group = findFrameGroup(&item);
        if (group && int(group->data) == m_filterFrame && (group->eventData == m_filterView3D || m_filterView3D == -1))
            result = QVariant::fromValue(QString("+"));
        else
            result = QVariant::fromValue(QString("-"));
        } break;
    }

    return result;
}

QVariant Quick3DFrameModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant result;
    if (orientation != Qt::Horizontal)
        return QAbstractItemModel::headerData(section, orientation, role);
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case Frame:
            result = QVariant::fromValue(Tr::tr("Frame"));
            break;
        case Duration:
            result = QVariant::fromValue(Tr::tr("Duration"));
            break;
        case Timestamp:
            result = QVariant::fromValue(Tr::tr("Timestamp"));
            break;
        case FrameDelta:
            result = QVariant::fromValue(Tr::tr("Frame Delta"));
            break;
        case View3D:
            result = QVariant::fromValue(Tr::tr("View3D"));
            break;
        }
        break;
    }
    return result;
}

QString Quick3DFrameModel::location(int index) const
{
    if (index < 0)
        return {};
    const Item &item = m_data[index];
    if (item.eventData == -1)
        return {};
    return m_modelManager->eventType(m_eventData[item.eventData]).data();
}

QModelIndex Quick3DFrameModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();
    if (parent.isValid()) {
        int parentIndex = parent.internalId();
        if (parentIndex >= m_data.size())
            return QModelIndex();
        const Item *parentData = &m_stackBottom;
        if (parentIndex >= 0)
            parentData = &m_data[parentIndex];
        return createIndex(row, column, parentData->children[row]);
    } else {
        return createIndex(row, column, row >= 0 ? m_stackBottom.children[row] : -1);
    }
}

int Quick3DFrameModel::parentRow(int index) const
{
    const Item *item = &m_data[index];
    const Item *parent = item->parent == -1 ? &m_stackBottom :  &m_data[item->parent];
    return parent->children.indexOf(index);
}

QModelIndex Quick3DFrameModel::parent(const QModelIndex &index) const
{
    if (index.isValid()) {
        int childIndex = index.internalId();
        const Item *childData = &m_data[childIndex];
        return childData->parent == -1 ? QModelIndex() : createIndex(parentRow(childData->parent), 0, childData->parent);
    } else {
        return QModelIndex();
    }
}

const Quick3DFrameModel::Item *Quick3DFrameModel::findFrameGroup(const Quick3DFrameModel::Item *item) const
{
    while (item) {
        if (item->additionalType == FrameGroup)
            return item;
        item = item->parent >= 0 ? &m_data[item->parent] : nullptr;
    }
    return nullptr;
}


void Quick3DFrameModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    int detailType = type.detailType();
    if (!m_acceptedDetailTypes.contains(detailType) || m_oldEvents)
        return;

    if (detailType == EventData){
        m_eventData.insert(m_eventData.size() + 1, event.typeIndex());
        return;
    }

    QList<quint64> numbers = event.numbers<QList<quint64>>();
    if (numbers.isEmpty())
        return;
    qint64 eventDuration = numbers[0];
    qint64 eventTime = event.timestamp() - eventDuration;
    quint64 data = numbers.size() > 1 ? numbers[1] : 0;
    QList<int> eventData;
    // The rest are pairs of event data id's
    for (int i = 2; i < numbers.size(); i++) {
        qint32 h = Quick3DModel::eventDataId(numbers[i] >> 32);
        qint32 l = Quick3DModel::eventDataId(numbers[i] & 0xffffffff);
        if (m_eventData.contains(h))
            eventData.push_back(h);
        if (m_eventData.contains(l))
            eventData.push_back(l);
    }
    if (eventData.isEmpty() && detailType == SynchronizeFrame) {
        m_oldEvents = true;
        return;
    }
    int index = m_data.size();
    Item item = Item(index, -1, eventTime, event.timestamp(), detailType, data);
    m_data.push_back(item);
    if (detailType == RenderCall) {
        for (int i = 0; i < eventData.size(); i++) {
            Item child = Item(index + i + 1, item.index, eventTime, event.timestamp(), SubData);
            child.eventData = eventData[i];
            m_data.push_back(child);
            m_data[index].children.push_back(child.index);
        }
    } else if (!eventData.isEmpty()) {
        m_data[index].eventData = eventData[0];
    }

    if (!(detailType >= RenderFrame && detailType <= PrepareFrame))
        return;

    int v3d = eventData[0];
    if (!m_frameTimes.contains(v3d))
        m_frameTimes[v3d] = {};

    switch (detailType) {
    case SynchronizeFrame:
        if (m_frameTimes[v3d].inFrame)
            qWarning () << "Previous frame was not ended correctly";
        m_frameTimes[v3d].begin = eventTime - 1;
        m_frameTimes[v3d].inFrame = true;
        break;
    case PrepareFrame:
        if (!m_frameTimes[v3d].inFrame) {
            qWarning () << "Synchronize frame missing";
            m_frameTimes[v3d].begin = eventTime - 1;
            m_frameTimes[v3d].inFrame = true;
        }
        m_frameTimes[v3d].prepareReceived = true;
        break;
    case RenderFrame:
        int index = m_data.size();
        if (!m_frameTimes[v3d].inFrame) {
            qWarning () << "Render message received without Synchronize and Prepare messages";
        } else {
            if (!m_frameTimes[v3d].prepareReceived)
                qWarning () << "Frame received without Prepare message";
            item = Item(index, -1, m_frameTimes[v3d].begin, event.timestamp() + 1, FrameGroup, m_frameTimes[v3d].frameCount);
            if (m_frameTimes[v3d].frameCount)
                item.frameDelta = item.end - m_frameTimes[v3d].end;
            item.eventData = v3d;
            m_frameTimes[v3d].frameCount++;
            m_frameTimes[v3d].end = item.end;
            m_frameTimes[v3d].inFrame = false;
            m_frameTimes[v3d].prepareReceived = false;
            m_data.push_back(item);
        }
        break;
    }
}

QStringList Quick3DFrameModel::view3DNames() const
{
    QStringList ret;
    for (auto value : m_frameTimes.keys())
        ret << m_modelManager->eventType(m_eventData[value]).data();
    return ret;
}

QList<int> Quick3DFrameModel::frameIndices(const QString &view3DFilter) const
{
    QList<int> ret;
    int key = -1;
    if (view3DFilter != Tr::tr("All", "All frames")) {
        for (int v3d : m_frameTimes.keys()) {
            if (m_modelManager->eventType(m_eventData[v3d]).data() == view3DFilter) {
                key = v3d;
                break;
            }
        }
    }

    for (auto child : m_stackBottom.children) {
        if (key == -1 || key == m_data[child].eventData)
            ret << child;
    }
    return ret;
}

QStringList Quick3DFrameModel::frameNames(const QString &view3D) const
{
    const QList<int> indices = frameIndices(view3D);
    QStringList ret;
    for (auto index : indices) {
        const Item &item = m_data[index];
        ret << QString(Tr::tr("Frame") + QLatin1Char(' ') + QString::number(item.data));
    }
    return ret;
}

void Quick3DFrameModel::setFilterFrame(const QString &frame)
{
    if (frame == Tr::tr("None", "Compare Frame: None")) {
        m_filterFrame = -1;
    } else {
        QString title = Tr::tr("Frame");
        QString number = frame.right(frame.size() - title.size());
        m_filterFrame = number.toInt();
    }
}

void Quick3DFrameModel::setFilterView3D(const QString &view3D)
{
    int key = -1;
    if (view3D != Tr::tr("All", "All View3D frames")) {
        for (int v3d : m_frameTimes.keys()) {
            if (m_modelManager->eventType(m_eventData[v3d]).data() == view3D) {
                key = v3d;
                break;
            }
        }
    }
    m_filterView3D = key;
}

void Quick3DFrameModel::finalize()
{
    if (m_oldEvents) {
        m_data.clear();
        endResetModel();
        return;
    }

    // Collect indices of items that need parent assignment.
    // SubData items already have parents set during loadEvent.
    QList<int> indices;
    indices.reserve(m_data.size());
    for (const auto &item : std::as_const(m_data)) {
        if (item.parent == -1)
            indices.push_back(item.index);
    }

    // Sort by begin time ascending; for equal begins, larger range (= parent) comes first.
    // FrameGroup items get begin = frame_begin - 1, so they naturally precede their children.
    std::sort(indices.begin(), indices.end(), [this](int a, int b) {
        const auto &ia = m_data[a];
        const auto &ib = m_data[b];
        if (ia.begin != ib.begin)
            return ia.begin < ib.begin;
        return ia.end > ib.end;
    });

    // Monotone stack: assign each item to the deepest open ancestor.
    QList<int> stack;
    for (int idx : indices) {
        Item &item = m_data[idx];
        while (!stack.isEmpty() && m_data[stack.back()].end <= item.begin)
            stack.pop_back();

        if (!stack.isEmpty()) {
            const int parentIdx = stack.back();
            item.parent = parentIdx;
            m_data[parentIdx].children.push_back(idx);
        } else {
            m_stackBottom.children.push_back(idx);
        }

        stack.push_back(idx);
    }

    endResetModel();
}

} // namespace Profiler::Internal
