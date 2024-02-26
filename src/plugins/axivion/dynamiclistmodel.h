// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractItemModel>
#include <QHash>
#include <QTimer>
#include <QVariant>

#include <optional>

namespace Axivion::Internal {

class ListItem
{
public:
    explicit ListItem(int row) : row(row) {}
    virtual ~ListItem() = default;
    virtual bool setData(int /*column*/, const QVariant &/*value*/, int /*role*/) { return false; }
    virtual QVariant data(int /*column*/, int /*role*/) const { return {}; }

    const int row;
};

class DynamicListModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit DynamicListModel(QObject *parent = nullptr);
    ~DynamicListModel();

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex&, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setItems(const QList<ListItem *> &items);
    void clear();

    void setExpectedRowCount(int expected);
    void setHeader(const QStringList &header);
    void setAlignments(const QList<Qt::Alignment> &alignments);

    QModelIndex indexForItem(const ListItem *item) const;

signals:
    void fetchRequested(int startRow, int limit);

private:
    void onNeedFetch(int row);
    void fetchNow();

    QHash<int, ListItem *> m_children;
    QStringList m_header;
    QList<Qt::Alignment> m_alignments;
    QTimer m_fetchMoreTimer;
    std::optional<int> m_expectedRowCount = {};
    int m_fetchStart = -1;
    int m_fetchEnd = -1;
    int m_lastFetch = -1;
    int m_lastFetchEnd = -1;
    int m_columnCount = 0;
};

} // namespace Axivion::Internal
