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

#include "perfprofilerstatisticsmodel.h"

#include <utils/qtcassert.h>

#include <QFileInfo>

namespace PerfProfiler {
namespace Internal {

static const char *headerLabels[] = {
    QT_TRANSLATE_NOOP("PerfProfilerStatisticsView", "Address"),
    QT_TRANSLATE_NOOP("PerfProfilerStatisticsView", "Function"),
    QT_TRANSLATE_NOOP("PerfProfilerStatisticsView", "Source Location"),
    QT_TRANSLATE_NOOP("PerfProfilerStatisticsView", "Binary Location"),
    QT_TRANSLATE_NOOP("PerfProfilerStatisticsView", "Caller"),
    QT_TRANSLATE_NOOP("PerfProfilerStatisticsView", "Callee"),
    QT_TRANSLATE_NOOP("PerfProfilerStatisticsView", "Occurrences"),
    QT_TRANSLATE_NOOP("PerfProfilerStatisticsView", "Occurrences in Percent"),
    QT_TRANSLATE_NOOP("PerfProfilerStatisticsView", "Recursion in Percent"),
    QT_TRANSLATE_NOOP("PerfProfilerStatisticsView", "Samples"),
    QT_TRANSLATE_NOOP("PerfProfilerStatisticsView", "Samples in Percent"),
    QT_TRANSLATE_NOOP("PerfProfilerStatisticsView", "Self Samples"),
    QT_TRANSLATE_NOOP("PerfProfilerStatisticsView", "Self in Percent")
};

Q_STATIC_ASSERT(sizeof(headerLabels) ==
                PerfProfilerStatisticsModel::MaximumColumn * sizeof(char *));

enum ColumnAvailability {
    AvailableInMain         = 1 << PerfProfilerStatisticsModel::Main,
    AvailableInChildren     = 1 << PerfProfilerStatisticsModel::Children,
    AvailableInParents      = 1 << PerfProfilerStatisticsModel::Parents,
    AvailableInRelatives    = AvailableInChildren | AvailableInParents,
    AvailableInAllRelations = AvailableInMain | AvailableInRelatives
};

static const ColumnAvailability columnsByRelation[] = {
    AvailableInAllRelations,
    AvailableInMain,
    AvailableInMain,
    AvailableInMain,
    AvailableInChildren,
    AvailableInParents,
    AvailableInAllRelations,
    AvailableInRelatives,
    AvailableInMain,
    AvailableInMain,
    AvailableInMain,
    AvailableInMain,
    AvailableInMain
};

Q_STATIC_ASSERT(sizeof(columnsByRelation) ==
                PerfProfilerStatisticsModel::MaximumColumn * sizeof(ColumnAvailability));

static inline bool operator<(const PerfProfilerStatisticsModel::Frame &a, int b)
{
    return a.typeId < b;
}

struct PerfProfilerStatisticsData
{
    QVector<PerfProfilerStatisticsMainModel::Data> mainData;
    QHash<int, PerfProfilerStatisticsRelativesModel::Data> parentsData;
    QHash<int, PerfProfilerStatisticsRelativesModel::Data> childrenData;
    uint totalSamples = 0;

    void loadEvent(const PerfEvent &event, const PerfEventType &type);
    void updateRelative(PerfProfilerStatisticsModel::Relation relation, const QVector<int> &stack);
    bool isEmpty() const;
    void clear();
};

PerfProfilerStatisticsModel::PerfProfilerStatisticsModel(Relation relation, QObject *parent) :
    QAbstractTableModel(parent), lastSortColumn(-1), lastSortOrder(Qt::AscendingOrder)
{
    m_font.setFamily(QLatin1String("Monospace"));
    for (int i = 0; i < MaximumColumn; ++i) {
        if (columnsByRelation[i] & (1 << relation))
            m_columns << static_cast<Column>(i);
    }
}

int PerfProfilerStatisticsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_columns.length();
}

void PerfProfilerStatisticsModel::resort()
{
    if (lastSortColumn != -1)
        sort(lastSortColumn, lastSortOrder);
}

QVariant PerfProfilerStatisticsModel::headerData(int section, Qt::Orientation orientation,
                                                 int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    return tr(headerLabels[m_columns[section]]);
}

void PerfProfilerStatisticsMainModel::resort()
{
    PerfProfilerStatisticsModel::resort();
    m_children->resort();
    m_parents->resort();
}

void PerfProfilerStatisticsMainModel::finalize(PerfProfilerStatisticsData *data)
{
    beginResetModel();

    data->mainData.swap(m_data);
    qSwap(m_totalSamples, data->totalSamples);

    int size = m_data.length();
    m_forwardIndex.resize(size);
    m_backwardIndex.resize(size);
    for (int i = 0; i < size; ++i) {
        m_forwardIndex[i] = i;
        m_backwardIndex[i] = i;
    }

    endResetModel();

    m_parents->finalize(data);
    m_children->finalize(data);

    resort();

    QTC_ASSERT(data->isEmpty(), data->clear());
    QTC_CHECK(m_offlineData.isNull());
    m_offlineData.reset(data);
}

int PerfProfilerStatisticsMainModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_data.length();
}

QByteArray PerfProfilerStatisticsMainModel::metaInfo(
        int typeId, PerfProfilerStatisticsModel::Column column) const
{
    // Need to look up stuff from tracemanager
    PerfProfilerTraceManager *manager = static_cast<PerfProfilerTraceManager *>(QObject::parent());
    switch (column) {
    case BinaryLocation:
    case Function: {
        const PerfProfilerTraceManager::Symbol &symbol
                = manager->symbol(manager->aggregateAddresses() ? typeId
                                                                : manager->symbolLocation(typeId));
        return manager->string(column == BinaryLocation ? symbol.binary : symbol.name);
    }
    case SourceLocation: {
        const PerfEventType::Location &location = manager->location(typeId);
        const QByteArray file = manager->string(location.file);
        return file.isEmpty()
                ? file
                : (QFileInfo(QLatin1String(file)).fileName().toUtf8() + ":"
                   + QByteArray::number(location.line));
    }
    default:
        return QByteArray();
    }
}

quint64 PerfProfilerStatisticsMainModel::address(int typeId) const
{
    PerfProfilerTraceManager *manager =
            static_cast<PerfProfilerTraceManager *>(QObject::parent());
    return manager->location(typeId).address;
}

void PerfProfilerStatisticsMainModel::initialize()
{
    // Make offline data unaccessible while we're loading events
    PerfProfilerStatisticsData *offline = m_offlineData.take();
    QTC_ASSERT(offline, return);
    QTC_ASSERT(offline->isEmpty(), offline->clear());
}

void PerfProfilerStatisticsData::loadEvent(const PerfEvent &event, const PerfEventType &type)
{
    if (event.timestamp() < 0)
        return;

    Q_UNUSED(type);
    ++totalSamples;
    auto data = mainData.end();
    const QVector<qint32> &stack = event.frames();
    for (auto typeId = stack.rbegin(), end = stack.rend(); typeId != end; ++typeId) {
        data = std::lower_bound(mainData.begin(), mainData.end(), *typeId);
        if (data == mainData.end() || data->typeId != *typeId)
            data = mainData.insert(data, PerfProfilerStatisticsMainModel::Data(*typeId)); // expensive
        ++(data->occurrences);

        auto prev = typeId;
        bool found = false;
        while (prev != stack.rbegin()) {
            if (*(--prev) == *typeId) {
                found = true;
                break;
            }
        }

        if (!found)
            ++(data->samples);
    }
    if (data != mainData.end())
        ++(data->self);

    updateRelative(PerfProfilerStatisticsModel::Children, stack);
    updateRelative(PerfProfilerStatisticsModel::Parents, stack);
}

QVariant PerfProfilerStatisticsMainModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::FontRole)
        return m_font;

    if (role != Qt::DisplayRole || !index.isValid())
        return QVariant();

    Column column = m_columns[index.column()];
    const Data &data = m_data.at(m_forwardIndex[index.row()]);

    switch (column) {
    case PerfProfilerStatisticsModel::Address:
        return address(data.typeId);
    case PerfProfilerStatisticsModel::Occurrences:
        return data.occurrences;
    case PerfProfilerStatisticsModel::RecursionInPercent:
        return static_cast<float>(1000 - data.samples * 1000 / data.occurrences) / 10.0f;
    case PerfProfilerStatisticsModel::Samples:
        return data.samples;
    case PerfProfilerStatisticsModel::SamplesInPercent:
        return static_cast<float>(data.samples * 1000 / m_totalSamples) / 10.0f;
    case PerfProfilerStatisticsModel::Self:
        return data.self;
    case PerfProfilerStatisticsModel::SelfInPercent:
        return static_cast<float>(data.self * 1000 / m_totalSamples) / 10.0f;
    default:
        return metaInfo(data.typeId, column);
    }
}

void PerfProfilerStatisticsMainModel::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();

    Column sortColumn = m_columns[column];
    std::sort(m_forwardIndex.begin(), m_forwardIndex.end(),
              [this, sortColumn, order](int a, int b) {
        auto pa = &m_data[order != Qt::AscendingOrder ? a : b];
        auto pb = &m_data[order != Qt::AscendingOrder ? b : a];
        switch (sortColumn) {
        case PerfProfilerStatisticsModel::Address:
            return address(pa->typeId) < address(pb->typeId);
        case PerfProfilerStatisticsModel::Occurrences:
            return pa->occurrences < pb->occurrences;
        case PerfProfilerStatisticsModel::RecursionInPercent:
            return pa->occurrences * 1000 / pa->samples <
                    pb->occurrences * 1000 / pb->samples;
        case PerfProfilerStatisticsModel::Samples:
        case PerfProfilerStatisticsModel::SamplesInPercent:
            return pa->samples < pb->samples;
        case PerfProfilerStatisticsModel::Self:
        case PerfProfilerStatisticsModel::SelfInPercent:
            return pa->self < pb->self;
        default:
            return metaInfo(pa->typeId, sortColumn) < metaInfo(pb->typeId, sortColumn);
        }
    });

    for (int i = 0; i < m_forwardIndex.length(); ++i)
        m_backwardIndex[m_forwardIndex[i]] = i;

    emit layoutChanged();
    lastSortColumn = column;
    lastSortOrder = order;
}


void PerfProfilerStatisticsMainModel::clear(PerfProfilerStatisticsData *data)
{
    beginResetModel();
    if (m_offlineData.isNull()) {
        // We didn't finalize
        data->clear();
        m_offlineData.reset(data);
    } else {
        QTC_CHECK(data == m_offlineData.data());
    }
    m_totalSamples = 0;
    m_data.clear();
    m_forwardIndex.clear();
    m_backwardIndex.clear();
    m_children->clear();
    m_parents->clear();
    m_startTime = std::numeric_limits<qint64>::min();
    m_endTime = std::numeric_limits<qint64>::max();
    endResetModel();
}

int PerfProfilerStatisticsMainModel::rowForTypeId(int typeId) const
{
    auto it = std::lower_bound(m_data.begin(), m_data.end(), typeId);
    if (it == m_data.end() || it->typeId != typeId)
        return -1;
    return m_backwardIndex[static_cast<int>(it - m_data.begin())];
}

PerfProfilerStatisticsMainModel::PerfProfilerStatisticsMainModel(PerfProfilerTraceManager *parent) :
    PerfProfilerStatisticsModel(Main, parent), m_startTime(std::numeric_limits<qint64>::min()),
    m_endTime(std::numeric_limits<qint64>::max()), m_totalSamples(0)
{
    m_children = new PerfProfilerStatisticsRelativesModel(Children, this);
    m_parents = new PerfProfilerStatisticsRelativesModel(Parents, this);
    PerfProfilerStatisticsData *data = new PerfProfilerStatisticsData;
    parent->registerFeatures(PerfEventType::attributeFeatures(),
                             std::bind(&PerfProfilerStatisticsData::loadEvent, data,
                                       std::placeholders::_1, std::placeholders::_2),
                             std::bind(&PerfProfilerStatisticsMainModel::initialize, this),
                             std::bind(&PerfProfilerStatisticsMainModel::finalize, this, data),
                             std::bind(&PerfProfilerStatisticsMainModel::clear, this, data));
    m_offlineData.reset(data);
}

PerfProfilerStatisticsMainModel::~PerfProfilerStatisticsMainModel()
{
    // If the offline data isn't here, we're being deleted while loading something. That's unnice.
    QTC_CHECK(!m_offlineData.isNull());
}

PerfProfilerStatisticsRelativesModel::PerfProfilerStatisticsRelativesModel(
        PerfProfilerStatisticsModel::Relation relation, PerfProfilerStatisticsMainModel *parent) :
    PerfProfilerStatisticsModel(relation, parent),
    m_relation(relation), m_currentRelative(-1)
{
}

int PerfProfilerStatisticsRelativesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_data.value(m_currentRelative).data.length();
}

QVariant PerfProfilerStatisticsRelativesModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::FontRole)
        return m_font;

    if (role != Qt::DisplayRole || !index.isValid())
        return QVariant();

    const Data &data = m_data.value(m_currentRelative);
    const Frame &row = data.data.at(index.row());
    Column column = m_columns[index.column()];
    switch (column) {
    case Address:
        return mainModel()->address(row.typeId);
    case Caller:
    case Callee:
        return mainModel()->metaInfo(row.typeId, Function);
    case Occurrences:
        return row.occurrences;
    case OccurrencesInPercent:
        return static_cast<float>(row.occurrences * 1000 / data.totalOccurrences) / 10.0f;
    default:
        return QVariant();
    }
}

void PerfProfilerStatisticsRelativesModel::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();

    Column sortColumn = m_columns[column];
    Data &data = m_data[m_currentRelative];
    std::sort(data.data.begin(), data.data.end(),
              [this, sortColumn, order](const Frame &a, const Frame &b) {
        auto pa = (order != Qt::AscendingOrder ? &a : &b);
        auto pb = (order != Qt::AscendingOrder ? &b : &a);
        switch (sortColumn) {
        case Address: {
            const PerfProfilerStatisticsMainModel *main = mainModel();
            return main->address(pa->typeId) < main->address(pb->typeId);
        }
        case Occurrences:
        case OccurrencesInPercent:
            return pa->occurrences < pb->occurrences;
        case Callee:
        case Caller: {
            const PerfProfilerStatisticsMainModel *main = mainModel();
            return main->metaInfo(pa->typeId, Function) < main->metaInfo(pb->typeId, Function);
        }
        default:
            return false;
        }
    });

    emit layoutChanged();
    lastSortColumn = column;
    lastSortOrder = order;
}

void PerfProfilerStatisticsRelativesModel::sortForInsert()
{
    emit layoutAboutToBeChanged();

    Data &data = m_data[m_currentRelative];
    std::sort(data.data.begin(), data.data.end(), [](const Frame &a, const Frame &b) {
        return a.typeId < b.typeId;
    });

    emit layoutChanged();
}

void PerfProfilerStatisticsRelativesModel::selectByTypeId(int typeId)
{
    if (typeId != m_currentRelative) {
        sortForInsert();
        beginResetModel();
        m_currentRelative = typeId;
        endResetModel();
        resort();
    }
}

void PerfProfilerStatisticsRelativesModel::finalize(PerfProfilerStatisticsData *data)
{
    beginResetModel();
    switch (m_relation) {
    case Children:
        m_data.swap(data->childrenData);
        QTC_ASSERT(data->childrenData.isEmpty(), data->childrenData.clear());
        break;
    case Parents:
        m_data.swap(data->parentsData);
        QTC_ASSERT(data->parentsData.isEmpty(), data->parentsData.clear());
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
    endResetModel();
    resort();
}

void PerfProfilerStatisticsData::updateRelative(PerfProfilerStatisticsModel::Relation relation,
                                                const QVector<int> &stack)
{
    int prevFrame = -1;
    const bool isParents = (relation == PerfProfilerStatisticsModel::Parents);
    auto &m_data = isParents ? parentsData : childrenData;
    for (auto frame = stack.rbegin(); frame != stack.rend() && *frame != -1; ++frame) {
        if (prevFrame != -1) {
            int currentType = isParents ? *frame : prevFrame;
            int relatedType = isParents ? prevFrame : *frame;

            PerfProfilerStatisticsRelativesModel::Data &relatedData = m_data[relatedType];
            auto row = std::lower_bound(relatedData.data.begin(), relatedData.data.end(),
                                        currentType);
            if (row == relatedData.data.end() || row->typeId != currentType)
                row = relatedData.data.insert(row, PerfProfilerStatisticsModel::Frame(currentType));
            ++(row->occurrences);
            ++relatedData.totalOccurrences;
        } else if (!isParents) {
            ++(m_data[*frame].totalOccurrences);
        }
        prevFrame = *frame;
    }
    if (prevFrame != -1 && isParents)
        ++(m_data[prevFrame].totalOccurrences);
}

bool PerfProfilerStatisticsData::isEmpty() const
{
    return mainData.isEmpty() && parentsData.isEmpty() && childrenData.isEmpty()
            && totalSamples == 0;
}

void PerfProfilerStatisticsData::clear()
{
    mainData.clear();
    parentsData.clear();
    childrenData.clear();
    totalSamples = 0;
}

void PerfProfilerStatisticsRelativesModel::clear()
{
    beginResetModel();
    m_data.clear();
    m_currentRelative = -1;
    endResetModel();
}

const PerfProfilerStatisticsMainModel *PerfProfilerStatisticsRelativesModel::mainModel() const
{
    return static_cast<const PerfProfilerStatisticsMainModel *>(parent());
}

} // namespace Internal
} // namespace PerfProfiler
