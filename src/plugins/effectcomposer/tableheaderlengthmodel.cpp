// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tableheaderlengthmodel.h"

TableHeaderLengthModel::TableHeaderLengthModel(QObject *parent)
    : QAbstractListModel(parent)
{}

QHash<int, QByteArray> TableHeaderLengthModel::roleNames() const
{
    static const QHash<int, QByteArray> result = {
        {LengthRole, "length"},
        {HiddenRole, "hidden"},
        {NameRole, "name"},
    };
    return result;
}

int TableHeaderLengthModel::rowCount(const QModelIndex &) const
{
    return m_data.size();
}

QVariant TableHeaderLengthModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (role) {
    case Roles::LengthRole:
        return m_data.at(index.row()).length;
    case Roles::HiddenRole:
        return !m_data.at(index.row()).visible;
    case Roles::NameRole:
        return m_sourceModel->headerData(index.row(), orientation(), Qt::DisplayRole);
    default:
        return {};
    }
}

bool TableHeaderLengthModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    if (data(index, role) == value)
        return false;

    switch (role) {
    case Roles::LengthRole: {
        m_data[index.row()].length = value.toInt();
        emit dataChanged(index, index, {role});
    } break;
    case Roles::HiddenRole: {
        m_data[index.row()].visible = !value.toBool();
        emit dataChanged(index, index, {role});
        emit sectionVisibilityChanged(index.row());
    } break;
    default:
        return false;
    }
    return true;
}

void TableHeaderLengthModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (m_sourceModel.get() == sourceModel)
        return;

    // Disconnect old model if exists
    if (m_sourceModel)
        disconnect(m_sourceModel.get(), 0, this, nullptr);

    m_sourceModel = sourceModel;
    emit sourceModelChanged();
    setupModel();
}

QAbstractItemModel *TableHeaderLengthModel::sourceModel() const
{
    return m_sourceModel;
}

void TableHeaderLengthModel::setOrientation(Qt::Orientation orientation)
{
    if (m_orientation == orientation)
        return;

    m_orientation = orientation;
    emit orientationChanged();
    setupModel();
}

Qt::Orientation TableHeaderLengthModel::orientation() const
{
    return m_orientation;
}

void TableHeaderLengthModel::onSourceItemsInserted(const QModelIndex &, int first, int last)
{
    beginInsertRows({}, first, last);
    const Item defaultItem{true, m_defaultLength};
    m_data.insert(first, last - first + 1, defaultItem);
    endInsertRows();
}

void TableHeaderLengthModel::onSourceItemsRemoved(const QModelIndex &, int first, int last)
{
    beginRemoveRows({}, first, last);
    m_data.remove(first, last - first + 1);
    endRemoveRows();
}

void TableHeaderLengthModel::onSourceItemsMoved(
    const QModelIndex &, int sourceStart, int sourceEnd, const QModelIndex &, int destinationRow)
{
    beginMoveRows({}, sourceStart, sourceEnd, {}, destinationRow);
    QList<Item> pack = m_data.mid(sourceStart, sourceEnd - sourceStart + 1);
    m_data.remove(sourceStart, sourceEnd - sourceStart + 1);
    while (!pack.isEmpty())
        m_data.insert(destinationRow, pack.takeLast());
    endMoveRows();
}

void TableHeaderLengthModel::checkModelReset()
{
    int availableItems = 0;
    if (m_sourceModel)
        availableItems = (orientation() == Qt::Horizontal) ? m_sourceModel->columnCount()
                                                           : m_sourceModel->rowCount();
    if (availableItems != rowCount())
        setupModel();
}

void TableHeaderLengthModel::setupModel()
{
    beginResetModel();
    m_data.clear();

    QAbstractItemModel *source = m_sourceModel.get();
    if (!source) {
        endResetModel();
        return;
    }

    disconnect(source, 0, this, nullptr);

    connect(source, &QAbstractItemModel::modelReset, this, &TableHeaderLengthModel::checkModelReset);

    const Item defaultItem{true, m_defaultLength};
    if (orientation() == Qt::Horizontal) {
        connect(
            source,
            &QAbstractItemModel::columnsInserted,
            this,
            &TableHeaderLengthModel::onSourceItemsInserted);

        connect(
            source,
            &QAbstractItemModel::columnsRemoved,
            this,
            &TableHeaderLengthModel::onSourceItemsRemoved);

        connect(
            source,
            &QAbstractItemModel::columnsMoved,
            this,
            &TableHeaderLengthModel::onSourceItemsMoved);

        m_data.insert(0, source->columnCount(), defaultItem);
    } else {
        connect(
            source,
            &QAbstractItemModel::rowsInserted,
            this,
            &TableHeaderLengthModel::onSourceItemsInserted);

        connect(
            source,
            &QAbstractItemModel::rowsRemoved,
            this,
            &TableHeaderLengthModel::onSourceItemsRemoved);

        connect(
            source,
            &QAbstractItemModel::rowsMoved,
            this,
            &TableHeaderLengthModel::onSourceItemsMoved);
        m_data.insert(0, source->rowCount(), defaultItem);
    }
    endResetModel();
}

bool TableHeaderLengthModel::invalidSection(int section) const
{
    return (section < 0 || section >= m_data.length());
}

int TableHeaderLengthModel::defaultLength() const
{
    return m_defaultLength;
}

void TableHeaderLengthModel::setDefaultLength(int value)
{
    if (m_defaultLength == value)
        return;
    m_defaultLength = value;
    emit defaultLengthChanged();
}

int TableHeaderLengthModel::length(int section) const
{
    if (invalidSection(section))
        return 0;
    return m_data.at(section).length;
}

void TableHeaderLengthModel::setLength(int section, int len)
{
    QModelIndex idx = index(section, 0);
    setData(idx, len, Roles::LengthRole);
}

bool TableHeaderLengthModel::isVisible(int section) const
{
    if (invalidSection(section))
        return false;

    return m_data.at(section).visible;
}

void TableHeaderLengthModel::setVisible(int section, bool visible)
{
    QModelIndex idx = index(section, 0);
    setData(idx, !visible, Roles::HiddenRole);
}

int TableHeaderLengthModel::minimumLength(int section) const
{
    if (invalidSection(section))
        return 0;

    // ToDo: handle it by data
    if (orientation() == Qt::Horizontal)
        return 70;

    return 20;
}
