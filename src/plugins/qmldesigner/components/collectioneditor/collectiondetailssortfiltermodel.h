// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QPointer>
#include <QSortFilterProxyModel>

namespace QmlDesigner {

class CollectionDetailsModel;

class CollectionDetailsSortFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(int selectedColumn READ selectedColumn WRITE selectColumn NOTIFY selectedColumnChanged)
    Q_PROPERTY(int selectedRow READ selectedRow WRITE selectRow NOTIFY selectedRowChanged)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)

    using Super = QSortFilterProxyModel;

public:
    explicit CollectionDetailsSortFilterModel(QObject *parent = nullptr);
    virtual ~CollectionDetailsSortFilterModel();

    void setSourceModel(CollectionDetailsModel *model);

    int selectedRow() const;
    int selectedColumn() const;

    Q_INVOKABLE bool selectRow(int row);
    Q_INVOKABLE bool selectColumn(int column);
    Q_INVOKABLE void deselectAll();

signals:
    void selectedColumnChanged(int);
    void selectedRowChanged(int);
    void isEmptyChanged(bool);

protected:
    using Super::setSourceModel;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &sourceleft, const QModelIndex &sourceRight) const override;

private:
    void updateEmpty();
    void updateSelectedRow();
    void updateSelectedColumn();
    void updateRowCountChanges();

    QPointer<CollectionDetailsModel> m_source;
    int m_selectedColumn = -1;
    int m_selectedRow = -1;
    bool m_isEmpty = true;
};

} // namespace QmlDesigner
