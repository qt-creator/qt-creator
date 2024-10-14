// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <designsystem/dsconstants.h>
#include <QAbstractItemModel>

#include <optional>

namespace QmlDesigner {
class DSThemeManager;
using PropInfo = std::pair<GroupType, PropertyName>;

class CollectionModel : public QAbstractItemModel
{
    enum class Roles { GroupRole = Qt::UserRole + 1, BindingRole };

public:
    CollectionModel(DSThemeManager *collection);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames() const override;

    void updateCache();

private:
    ThemeId findThemeId(int column) const;
    std::optional<PropInfo> findPropertyName(int row) const;

private:
    DSThemeManager *m_collection = nullptr;

    // cache
    std::vector<ThemeId> m_themeIdList;
    std::vector<PropInfo> m_propertyInfoList;
};
} // namespace QmlDesigner
