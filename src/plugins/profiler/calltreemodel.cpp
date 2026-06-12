// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "calltreemodel.h"

#include "profilertr.h"

#include <QHash>

#include <algorithm>

using namespace Profiler;

namespace QmlProfiler::Internal {

CallTreeModel::CallTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_root(std::make_unique<Node>())
{}

CallTreeModel::~CallTreeModel() = default;

void CallTreeModel::setTraceData(const SampleTraceData *data)
{
    m_data = data;
    m_startUs = -1;
    m_endUs = -1;
    rebuild();
}

void CallTreeModel::setTimeRange(qint64 startUs, qint64 endUs)
{
    if (m_startUs == startUs && m_endUs == endUs)
        return;
    m_startUs = startUs;
    m_endUs = endUs;
    rebuild();
}

static void sortByWeight(CallTreeModel::Node *node)
{
    std::sort(node->children.begin(), node->children.end(),
              [](const std::unique_ptr<CallTreeModel::Node> &a,
                 const std::unique_ptr<CallTreeModel::Node> &b) {
                  if (a->weight != b->weight)
                      return a->weight > b->weight;
                  return a->labelId < b->labelId; // deterministic order for equal weights
              });
    for (size_t i = 0; i < node->children.size(); ++i) {
        node->children[i]->row = int(i);
        sortByWeight(node->children[i].get());
    }
}

void CallTreeModel::rebuild()
{
    beginResetModel();
    m_root = std::make_unique<Node>();
    if (m_data) {
        // Children are located by label id while building; the lookup maps are
        // discarded once the tree is sorted.
        QHash<const Node *, QHash<int, Node *>> childByLabel;
        for (const SampleTraceData::ThreadSample &sample : m_data->samples) {
            if (!sample.running || sample.frames.isEmpty())
                continue;
            if (m_startUs >= 0 && qint64(sample.tsUs) < m_startUs)
                continue;
            if (m_endUs >= 0 && qint64(sample.tsUs) > m_endUs)
                continue;
            ++m_root->weight;
            Node *node = m_root.get();
            for (int labelId : sample.frames) {
                Node *&child = childByLabel[node][labelId];
                if (!child) {
                    auto owned = std::make_unique<Node>();
                    owned->labelId = labelId;
                    owned->parent = node;
                    child = owned.get();
                    node->children.push_back(std::move(owned));
                }
                ++child->weight;
                node = child;
            }
            ++node->self;
        }
        sortByWeight(m_root.get());
    }
    endResetModel();
}

quint64 CallTreeModel::totalWeight() const
{
    return m_root->weight;
}

const CallTreeModel::Node *CallTreeModel::node(const QModelIndex &index) const
{
    return index.isValid() ? static_cast<const Node *>(index.internalPointer()) : nullptr;
}

QModelIndex CallTreeModel::indexFor(const Node *node, int column) const
{
    if (!node || node == m_root.get())
        return {};
    return createIndex(node->row, column, const_cast<Node *>(node));
}

QString CallTreeModel::symbol(const Node *node) const
{
    if (!m_data || !node || node->labelId < 0 || node->labelId >= m_data->labels.size())
        return {};
    return m_data->labels.at(node->labelId);
}

QList<const CallTreeModel::Node *> CallTreeModel::heaviestPath(const Node *from) const
{
    QList<const Node *> path;
    const Node *node = from;
    if (!node && !m_root->children.empty())
        node = m_root->children.front().get();
    for (; node; node = node->children.empty() ? nullptr : node->children.front().get())
        path.append(node);
    return path;
}

QModelIndex CallTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    const Node *parentNode = parent.isValid() ? node(parent) : m_root.get();
    if (!parentNode || row < 0 || row >= int(parentNode->children.size()) || column < 0
        || column >= ColumnCount) {
        return {};
    }
    return createIndex(row, column, parentNode->children[size_t(row)].get());
}

QModelIndex CallTreeModel::parent(const QModelIndex &child) const
{
    const Node *childNode = node(child);
    if (!childNode || !childNode->parent || childNode->parent == m_root.get())
        return {};
    return indexFor(childNode->parent);
}

int CallTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return 0;
    const Node *parentNode = parent.isValid() ? node(parent) : m_root.get();
    return parentNode ? int(parentNode->children.size()) : 0;
}

int CallTreeModel::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}

QVariant CallTreeModel::data(const QModelIndex &index, int role) const
{
    const Node *n = node(index);
    if (!n)
        return {};
    if (role == Qt::TextAlignmentRole && index.column() != SymbolColumn)
        return int(Qt::AlignRight | Qt::AlignVCenter);
    if (role != Qt::DisplayRole)
        return {};
    switch (index.column()) {
    case WeightColumn: {
        const quint64 total = totalWeight();
        const double percent = total ? 100.0 * double(n->weight) / double(total) : 0.0;
        return QString::fromLatin1("%1 (%2%)").arg(n->weight).arg(percent, 0, 'f', 1);
    }
    case SelfColumn:
        return QString::number(n->self);
    case SymbolColumn:
        return symbol(n);
    }
    return {};
}

QVariant CallTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case WeightColumn: return Tr::tr("Weight");
    case SelfColumn: return Tr::tr("Self");
    case SymbolColumn: return Tr::tr("Symbol");
    }
    return {};
}

} // namespace QmlProfiler::Internal
