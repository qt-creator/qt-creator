/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "perfprofilerflamegraphmodel.h"

#include <QFileInfo>
#include <QQueue>
#include <QPointer>

#include <unordered_map>

namespace PerfProfiler {
namespace Internal {

class Payload
{
public:
    Payload(const PerfProfilerFlameGraphData *parent, PerfProfilerFlameGraphModel::Data *data,
            int numSamples)
        : m_parent(parent), m_data(data), m_numSamples(numSamples)
    {}

    Payload(const Payload &other) = delete;
    Payload &operator=(const Payload &other) = delete;

    Payload(Payload &&other) = default;
    Payload &operator=(Payload &&other) = default;

    void adjust(qint64 diff);
    void countObservedRelease();
    void countGuessedRelease();
    void countObservedAllocation();
    void countLostRequest();

private:
    const PerfProfilerFlameGraphData *m_parent;
    PerfProfilerFlameGraphModel::Data *m_data;
    const int m_numSamples;
};

typedef PerfResourceCounter<Payload> ThreadResourceCounter;

class ProcessResourceCounter
{
public:
    ThreadResourceCounter &operator[](quint32 tid)
    {
        auto it = m_counters.find(tid);
        if (it == m_counters.end())
            it = m_counters.emplace(tid, ThreadResourceCounter(&m_container)).first;
        return it->second;
    }

private:
    std::unordered_map<quint32, ThreadResourceCounter> m_counters;
    ThreadResourceCounter::Container m_container;
};

struct PerfProfilerFlameGraphData
{

    PerfProfilerFlameGraphData() { clear(); }

    void loadEvent(const PerfEvent &event, const PerfEventType &type);
    PerfProfilerFlameGraphModel::Data *pushChild(PerfProfilerFlameGraphModel::Data *parent,
                                                 int typeId, int numSamples);
    void updateTraceData(const PerfEvent &event, const PerfEventType &type,
                         PerfProfilerFlameGraphModel::Data *data, int numSamples);
    void clear();
    bool isEmpty() const;

    QScopedPointer<PerfProfilerFlameGraphModel::Data> stackBottom;
    std::unordered_map<quint32, ProcessResourceCounter> resourceBlocks;
    QPointer<const PerfProfilerTraceManager> manager;
    uint resourcePeakId = 0;
};

PerfProfilerFlameGraphModel::PerfProfilerFlameGraphModel(PerfProfilerTraceManager *manager) :
    QAbstractItemModel(manager), m_stackBottom(new Data(nullptr, -1, 0))
{
    PerfProfilerFlameGraphData *data = new PerfProfilerFlameGraphData;
    manager->registerFeatures(PerfEventType::attributeFeatures(),
                              std::bind(&PerfProfilerFlameGraphData::loadEvent, data,
                                        std::placeholders::_1, std::placeholders::_2),
                              std::bind(&PerfProfilerFlameGraphModel::initialize, this),
                              std::bind(&PerfProfilerFlameGraphModel::finalize, this, data),
                              std::bind(&PerfProfilerFlameGraphModel::clear, this, data));
    m_offlineData.reset(data);
}

PerfProfilerFlameGraphModel::~PerfProfilerFlameGraphModel()
{
    // If the offline data isn't here, we're being deleted while loading something. That's unnice.
    QTC_CHECK(!m_offlineData.isNull());
}

QModelIndex PerfProfilerFlameGraphModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        Data *parentData = static_cast<Data *>(parent.internalPointer());
        return createIndex(row, column, parentData->children[row]);
    } else {
        return createIndex(row, column, row >= 0 ? m_stackBottom->children[row] : nullptr);
    }
}

QModelIndex PerfProfilerFlameGraphModel::parent(const QModelIndex &child) const
{
    if (child.isValid()) {
        Data *childData = static_cast<Data *>(child.internalPointer());
        return childData->parent == m_stackBottom.data() ? QModelIndex()
                                                         : createIndex(0, 0, childData->parent);
    } else {
        return QModelIndex();
    }
}

int PerfProfilerFlameGraphModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        Data *parentData = static_cast<Data *>(parent.internalPointer());
        return parentData->children.count();
    } else {
        return m_stackBottom->children.count();
    }
}

int PerfProfilerFlameGraphModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

static const QByteArray &orUnknown(const QByteArray &string)
{
    static const QByteArray unknown = PerfProfilerFlameGraphModel::tr("[unknown]").toUtf8();
    return string.isEmpty() ? unknown : string;
}

QVariant PerfProfilerFlameGraphModel::data(const QModelIndex &index, int role) const
{
    const Data *data = static_cast<Data *>(index.internalPointer());
    if (!data)
        data = m_stackBottom.data();

    switch (role) {
    case TypeIdRole:
        return data->typeId;
    case SamplesRole:
        return data->samples;
    case ObservedResourceAllocationsRole:
        return data->observedResourceAllocations;
    case LostResourceRequestsRole:
        return data->lostResourceRequests;
    case ResourceAllocationsRole:
        return data->observedResourceAllocations + data->lostResourceRequests;
    case ObservedResourceReleasesRole:
        return data->observedResourceReleases;
    case GuessedResourceReleasesRole:
        return data->guessedResourceReleases;
    case ResourceReleasesRole:
        return data->observedResourceReleases + data->guessedResourceReleases;
    case ResourcePeakRole:
        return data->resourcePeak;
    default: break;
    }

    // If it's not a location, we cannot provide any details.
    if (data->typeId < 0)
        return QVariant();

    // Need to look up stuff from modelmanager
    PerfProfilerTraceManager *manager =
            static_cast<PerfProfilerTraceManager *>(QObject::parent());
    const bool aggregated = manager->aggregateAddresses();
    const PerfProfilerTraceManager::Symbol &symbol
            = manager->symbol(aggregated ? data->typeId
                                         : manager->symbolLocation(data->typeId));
    const PerfEventType::Location &location = manager->location(data->typeId);
    switch (role) {
    case DisplayNameRole:
        return QString::fromLatin1("0x%1").arg(location.address, 16, 16, QLatin1Char('0'));
    case FunctionRole:
        return orUnknown(manager->string(symbol.name));
    case ElfFileRole:
        return orUnknown(manager->string(symbol.binary));
    case SourceFileRole:
        return orUnknown(manager->string(location.file));
    case LineRole:
        return location.line;
    default:
        return QVariant();
    }
}

void PerfProfilerFlameGraphModel::initialize()
{
    PerfProfilerFlameGraphData *offline = m_offlineData.take();
    QTC_ASSERT(offline, return);
    QTC_ASSERT(offline->isEmpty(), offline->clear());
    offline->manager = static_cast<PerfProfilerTraceManager *>(QObject::parent());
}

void PerfProfilerFlameGraphData::updateTraceData(const PerfEvent &event, const PerfEventType &type,
                                                 PerfProfilerFlameGraphModel::Data *data,
                                                 int numSamples)
{
    const PerfEventType::Attribute &attribute = type.attribute();
    if (attribute.type == PerfEventType::TypeTracepoint) {
        const PerfProfilerTraceManager::TracePoint &tracePoint
                = manager->tracePoint(static_cast<int>(attribute.config));

        const QByteArray &name = manager->string(tracePoint.name);
        if (name.startsWith(PerfProfilerTraceManager::s_resourceNamePrefix)) {
            const QHash<qint32, QVariant> &traceData = event.traceData();
            const auto end = traceData.end();

            const auto released = traceData.find(manager->resourceReleasedIdId());
            const auto amount = traceData.find(manager->resourceRequestedAmountId());
            const auto obtained = traceData.find(manager->resourceObtainedIdId());
            const auto moved = traceData.find(manager->resourceMovedIdId());

            auto &threadCounter = resourceBlocks[event.pid()][event.tid()];
            Payload payload(this, data, numSamples);

            if (amount != end) {
                const auto blocks = traceData.find(manager->resourceRequestedBlocksId());
                const qint64 amountValue = amount.value().toLongLong()
                        * (blocks == end ? 1 : blocks.value().toLongLong());

                if (amountValue < 0) { // integer overflow
                    qWarning() << "Excessively large allocation detected in trace point";
                } else if (released == end) {
                    threadCounter.request(amountValue, std::move(payload));
                } else {
                    threadCounter.request(amountValue, released.value().toULongLong(),
                                          std::move(payload));
                }
            } else if (released != end) {
                threadCounter.release(released.value().toULongLong(), std::move(payload));
            } else if (obtained != end) {
                threadCounter.obtain(obtained.value().toULongLong(), std::move(payload));
            } else if (moved != end) {
                threadCounter.move(moved.value().toULongLong(), std::move(payload));
            }

            if (stackBottom->resourceUsage > stackBottom->resourcePeak)
                resourcePeakId += numSamples;
        }
    }
}

void PerfProfilerFlameGraphData::clear()
{
    if (!stackBottom || !stackBottom->isEmpty())
        stackBottom.reset(new PerfProfilerFlameGraphModel::Data(nullptr, -1, 0));
    resourceBlocks.clear();
    manager.clear();
    resourcePeakId = 0;
}

bool PerfProfilerFlameGraphData::isEmpty() const
{
    return stackBottom->isEmpty() && resourceBlocks.empty() && manager.isNull()
            && resourcePeakId == 0;
}

void PerfProfilerFlameGraphData::loadEvent(const PerfEvent &event, const PerfEventType &type)
{
    const int numSamples = (event.timestamp() < 0) ? 0 : 1;
    stackBottom->samples += numSamples;
    auto data = stackBottom.data();
    const QVector<int> &stack = event.frames();
    for (auto it = stack.rbegin(), end = stack.rend(); it != end; ++it)
        data = pushChild(data, *it, numSamples);

    updateTraceData(event, type, data, numSamples);
}

void PerfProfilerFlameGraphModel::finalize(PerfProfilerFlameGraphData *data)
{
    beginResetModel();

    m_stackBottom.swap(data->stackBottom);

    QQueue<Data *> nodes;
    nodes.enqueue(m_stackBottom.data());
    while (!nodes.isEmpty()) {
        Data *node = nodes.dequeue();
        if (node->lastResourceChangeId < data->resourcePeakId) {
            node->resourcePeak = node->resourceUsage;
            node->lastResourceChangeId = data->resourcePeakId;
        }
        for (Data *child : qAsConst(node->children))
            nodes.enqueue(child);
    }

    endResetModel();

    QTC_CHECK(data->stackBottom->isEmpty());
    data->clear();
    m_offlineData.reset(data);
}

void PerfProfilerFlameGraphModel::clear(PerfProfilerFlameGraphData *data)
{
    beginResetModel();
    if (m_offlineData.isNull()) {
        // We didn't finalize
        data->clear();
        m_offlineData.reset(data);
    } else {
        QTC_CHECK(data == m_offlineData.data());
    }
    m_stackBottom.reset(new Data(nullptr, -1, 0));
    endResetModel();
}

PerfProfilerFlameGraphModel::Data *PerfProfilerFlameGraphData::pushChild(
        PerfProfilerFlameGraphModel::Data *parent, int typeId, int numSamples)
{
    QVector<PerfProfilerFlameGraphModel::Data *> &siblings = parent->children;

    for (auto it = siblings.begin(), end = siblings.end(); it != end; ++it) {
        PerfProfilerFlameGraphModel::Data *child = *it;
        if (child->typeId == typeId) {
            child->samples += numSamples;
            for (auto back = it, front = siblings.begin(); back != front;) {
                --back;
                if ((*back)->samples >= (*it)->samples)
                    break;
                qSwap(*it, *back);
                it = back;
            }
            return child;
        }
    }

    PerfProfilerFlameGraphModel::Data *child
            = new PerfProfilerFlameGraphModel::Data(parent, typeId, numSamples);
    parent->children.append(child);
    return child;
}

void Payload::adjust(qint64 diff)
{
    for (auto allocator = m_data; allocator; allocator = allocator->parent) {
        if (allocator->lastResourceChangeId < m_parent->resourcePeakId)
            allocator->resourcePeak = allocator->resourceUsage;

        allocator->lastResourceChangeId = m_parent->resourcePeakId;
        allocator->resourceUsage += diff;
    }
}

void Payload::countObservedRelease()
{
    for (auto allocator = m_data; allocator; allocator = allocator->parent)
        allocator->observedResourceReleases += m_numSamples;
}

void Payload::countGuessedRelease()
{
    for (auto allocator = m_data; allocator; allocator = allocator->parent)
        allocator->guessedResourceReleases += m_numSamples;
}

void Payload::countObservedAllocation()
{
    for (auto allocator = m_data; allocator; allocator = allocator->parent)
        allocator->observedResourceAllocations += m_numSamples;
}

void Payload::countLostRequest()
{
    for (auto allocator = m_data; allocator; allocator = allocator->parent)
        allocator->lostResourceRequests += m_numSamples;
}

} // namespace Internal
} // namespace PerfProfiler
