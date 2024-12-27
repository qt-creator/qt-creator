// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectcomposeruniformstablemodel.h"

#include "uniform.h"

#include <utils/algorithm.h>

#include <QColor>
#include <QHash>
#include <QVector4D>

namespace EffectComposer {

namespace {

struct Tr
{
    static inline QString tr(const auto &text)
    {
        return EffectComposerUniformsTableModel::tr(text);
    }
};

struct RoleColMap
{
    using UniformRole = EffectComposerUniformsModel::Roles;

    struct UniformRoleData
    {
        UniformRole role;
        QString description;
    };

    static QList<UniformRoleData> tableCols()
    {
        static const QList<UniformRoleData> list{
            {UniformRole::NameRole, Tr::tr("Uniform Name")},
            {UniformRole::DisplayNameRole, Tr::tr("Property Name")},
            {UniformRole::TypeRole, Tr::tr("Type")},
            {UniformRole::MinValueRole, Tr::tr("Min")},
            {UniformRole::MaxValueRole, Tr::tr("Max")},
            {UniformRole::DescriptionRole, Tr::tr("Description")},
        };
        return list;
    };

    static UniformRole colToRole(int col)
    {
        static const QList<UniformRoleData> &list = tableCols();
        return list.at(col).role;
    }

    static int roleToCol(int role)
    {
        static const QHash<int, int> colOfRole = [] {
            QHash<int, int> result;
            int col = -1;
            const QList<UniformRoleData> &columns = tableCols();
            for (const UniformRoleData &roleData : columns)
                result.insert(roleData.role, ++col);
            return result;
        }();
        return colOfRole.value(role, -1);
    }

    static QString colDescription(int col) { return tableCols().at(col).description; }
};

struct ValueDisplay
{
    QString operator()(const QVector2D &vec) const
    {
        return QString("(%1, %2)").arg(vec.x()).arg(vec.y());
    }

    QString operator()(const QVector3D &vec) const
    {
        return QString("(%1, %2, %3)").arg(vec.x()).arg(vec.y()).arg(vec.z());
    }

    QString operator()(const QVector4D &vec) const
    {
        return QString("(%1, %2, %3, %4)").arg(vec.x()).arg(vec.y()).arg(vec.z()).arg(vec.w());
    }

    QString operator()(const QColor &color) const
    {
        return QString("(%1, %2, %3, %4)")
            .arg(color.redF())
            .arg(color.greenF())
            .arg(color.blueF())
            .arg(color.alphaF());
    }

    QString operator()(const auto &) const { return Tr::tr("Unsupported type"); }
};

bool isValueTypedRole(EffectComposerUniformsModel::Roles role)
{
    using UniformRole = EffectComposerUniformsModel::Roles;
    switch (role) {
    case UniformRole::ValueRole:
    case UniformRole::DefaultValueRole:
    case UniformRole::MinValueRole:
    case UniformRole::MaxValueRole:
        return true;
    default:
        return false;
    }
}

} // namespace

EffectComposerUniformsTableModel::EffectComposerUniformsTableModel(
    EffectComposerUniformsModel *sourceModel, QObject *parent)
    : QAbstractTableModel(parent)
    , m_sourceModel(sourceModel)
{
    if (!sourceModel)
        return;

    connect(
        sourceModel,
        &QAbstractItemModel::modelAboutToBeReset,
        this,
        &EffectComposerUniformsTableModel::modelAboutToBeReset);
    connect(
        sourceModel,
        &QAbstractItemModel::modelReset,
        this,
        &EffectComposerUniformsTableModel::modelReset);
    connect(
        sourceModel,
        &QAbstractItemModel::rowsAboutToBeInserted,
        this,
        &EffectComposerUniformsTableModel::onSourceRowsAboutToBeInserted);
    connect(
        sourceModel,
        &QAbstractItemModel::rowsInserted,
        this,
        &EffectComposerUniformsTableModel::endInsertRows);
    connect(
        sourceModel,
        &QAbstractItemModel::rowsAboutToBeRemoved,
        this,
        &EffectComposerUniformsTableModel::onSourceRowsAboutToBeRemoved);
    connect(
        sourceModel,
        &QAbstractItemModel::rowsRemoved,
        this,
        &EffectComposerUniformsTableModel::endRemoveRows);

    connect(
        sourceModel,
        &QAbstractItemModel::rowsAboutToBeMoved,
        this,
        &EffectComposerUniformsTableModel::onSourceRowsAboutToBeMoved);
    connect(
        sourceModel,
        &QAbstractItemModel::rowsMoved,
        this,
        &EffectComposerUniformsTableModel::endMoveRows);
    connect(
        sourceModel,
        &QAbstractItemModel::dataChanged,
        this,
        &EffectComposerUniformsTableModel::onSourceDataChanged);

    connect(sourceModel, &QObject::destroyed, this, &QObject::deleteLater);
}

EffectComposerUniformsTableModel::SourceIndex EffectComposerUniformsTableModel::mapToSource(
    const QModelIndex &proxyIndex) const
{
    if (!m_sourceModel)
        return {};

    return {m_sourceModel->index(proxyIndex.row(), 0), RoleColMap::colToRole(proxyIndex.column())};
}

QHash<int, QByteArray> EffectComposerUniformsTableModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"},
        {Role::ValueRole, "value"},
        {Role::ValueTypeRole, "valueType"},
        {Role::CanCopyRole, "canCopy"},
        {Role::IsDescriptionRole, "isDescription"},
    };
}

int EffectComposerUniformsTableModel::rowCount(const QModelIndex &) const
{
    if (!m_sourceModel)
        return 0;

    return m_sourceModel->rowCount();
}

int EffectComposerUniformsTableModel::columnCount(const QModelIndex &) const
{
    return RoleColMap::tableCols().size();
}

QVariant EffectComposerUniformsTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (role) {
    case Role::ValueRole:
        return mapToSource(index).value();

    case Qt::DisplayRole:
        return mapToSource(index).display();

    case Role::ValueTypeRole:
        return mapToSource(index).valueTypeString();

    case Role::CanCopyRole:
        return mapToSource(index).role == UniformRole::NameRole;

    case Role::IsDescriptionRole:
        return mapToSource(index).role == UniformRole::DescriptionRole;

    default:
        return {};
    }
}

QVariant EffectComposerUniformsTableModel::headerData(
    int section, Qt::Orientation orientation, int) const
{
    if (orientation == Qt::Vertical) {
        if (section >= 0 && section < rowCount())
            return section;
    } else if (orientation == Qt::Horizontal) {
        if (section >= 0 && section < columnCount())
            return RoleColMap::colDescription(section);
    }
    return {};
}

EffectComposerUniformsModel *EffectComposerUniformsTableModel::sourceModel() const
{
    return m_sourceModel.get();
}

void EffectComposerUniformsTableModel::onSourceRowsAboutToBeInserted(
    const QModelIndex &, int first, int last)
{
    beginInsertRows({}, first, last);
}

void EffectComposerUniformsTableModel::onSourceRowsAboutToBeRemoved(
    const QModelIndex &, int first, int last)
{
    beginRemoveRows({}, first, last);
}

void EffectComposerUniformsTableModel::onSourceRowsAboutToBeMoved(
    const QModelIndex &, int sourceStart, int sourceEnd, const QModelIndex &, int destinationRow)
{
    beginMoveRows({}, sourceStart, sourceEnd, {}, destinationRow);
}

void EffectComposerUniformsTableModel::onSourceDataChanged(
    const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    const int startRow = topLeft.row();
    const int endRow = bottomRight.row();

    int minCol = 0;
    int maxCol = 0;
    if (roles.empty()) {
        minCol = 0;
        maxCol = columnCount();
    } else {
        minCol = std::numeric_limits<int>::max();
        maxCol = std::numeric_limits<int>::min();
        for (int role : roles) {
            const int col = RoleColMap::roleToCol(role);
            if (col < 0)
                continue;
            minCol = std::min(minCol, col);
            maxCol = std::max(maxCol, col);
        }
    }
    emit dataChanged(index(startRow, minCol), index(endRow, maxCol));
}

QString EffectComposerUniformsTableModel::SourceIndex::valueTypeString() const
{
    using namespace Qt::StringLiterals;
    if (isValueTypedRole(role))
        return index.data(UniformRole::TypeRole).toString();

    return "string"_L1;
}

QString EffectComposerUniformsTableModel::SourceIndex::display() const
{
    // For descriptive types, we can return string value
    if (!isValueTypedRole(role))
        return value().toString();

    // For value types, we should stringify them based on uniform type
    const QString uniformTypeStr = index.data(UniformRole::TypeRole).toString();
    Uniform::Type uniformType = Uniform::typeFromString(uniformTypeStr);
    const QVariant val = value();
    switch (uniformType) {
    case Uniform::Type::Vec2:
        return ValueDisplay{}(val.value<QVector2D>());
    case Uniform::Type::Vec3:
        return ValueDisplay{}(val.value<QVector3D>());
    case Uniform::Type::Vec4:
        return ValueDisplay{}(val.value<QVector4D>());
    case Uniform::Type::Color:
        return ValueDisplay{}(val.value<QColor>());
    default:
        return val.toString();
    }
}

} // namespace EffectComposer
