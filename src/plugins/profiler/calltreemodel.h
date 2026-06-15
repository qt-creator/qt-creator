// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sampletrace.h"

#include <QAbstractItemModel>

#include <memory>
#include <vector>

namespace QmlProfiler::Internal {

// Aggregates the on-CPU samples of a recording into one merged call tree:
// every sample's root-first frame path is merged so identical prefixes share
// nodes, and the roots are the distinct first calls across all threads.
// Children are sorted heaviest-first.
class CallTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    // The symbol is the tree column, so it comes first.
    enum Column { SymbolColumn, WeightColumn, SelfColumn, ColumnCount };

    struct Node
    {
        int labelId = -1;
        quint64 weight = 0; // on-CPU samples whose path contains this node
        quint64 self = 0;   // on-CPU samples whose innermost frame is this node
        Node *parent = nullptr;
        int row = 0; // index in parent->children after sorting
        std::vector<std::unique_ptr<Node>> children; // heaviest first, ties by label id

        int childCount() const { return int(children.size()); }
        const Node *child(int i) const { return children[size_t(i)].get(); }
    };

    explicit CallTreeModel(QObject *parent = nullptr);
    ~CallTreeModel() override;

    // The model keeps the pointer; pass nullptr to clear. Resets the time range.
    void setTraceData(const SampleTraceData *data);

    // Restrict aggregation to samples with startUs <= tsUs <= endUs.
    // Negative values mean "no restriction".
    void setTimeRange(qint64 startUs, qint64 endUs);

    quint64 totalWeight() const; // counted on-CPU samples in the current range

    // Node pointers handed out below remain valid only until the next rebuild
    // (setTraceData() or setTimeRange()); the accompanying model reset is the
    // signal to drop them.
    const Node *node(const QModelIndex &index) const; // nullptr for invalid
    QModelIndex indexFor(const Node *node, int column = 0) const;
    QString symbol(const Node *node) const;

    struct SourceLocation { QString file; int line = 0; };
    // The node's representative source location, or an empty file when the
    // frame has no debug info. Valid until the next rebuild, like node().
    SourceLocation location(const Node *node) const;

    // The chain of heaviest children starting at `from` (or at the heaviest
    // root when `from` is null), including the starting node itself.
    QList<const Node *> heaviestPath(const Node *from = nullptr) const;

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    void rebuild();

    const SampleTraceData *m_data = nullptr;
    qint64 m_startUs = -1;
    qint64 m_endUs = -1;
    std::unique_ptr<Node> m_root; // invisible; its children are the tree roots,
                                  // its weight is the total sample count
};

} // namespace QmlProfiler::Internal
