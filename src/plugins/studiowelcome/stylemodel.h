// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QStandardItemModel>

namespace StudioWelcome {

class StyleModel : public QAbstractListModel
{
    Q_OBJECT

private:
    using Items = std::vector<QStandardItem *>;

public:
    explicit StyleModel(QObject *parent = nullptr);

    Q_INVOKABLE QString iconId(int index) const;
    Q_INVOKABLE void filter(const QString &what = "all");

    int rowCount(const QModelIndex &/*parent*/) const override
    {
        if (m_backendModel)
            return static_cast<int>(m_filteredItems.size());

        return 0;
    }

    QVariant data(const QModelIndex &index, int /*role*/) const override
    {
        if (m_backendModel) {
            auto *item = m_filteredItems.at(index.row());
            return item->text();
        }

        return {};
    }

    QHash<int, QByteArray> roleNames() const override
    {
        if (m_backendModel)
            return m_roles;

        /**
         * TODO: roleNames is called before the backend model is loaded, which may be buggy. But I
         * found no way to force refresh the model, so as to reload the role names afterwards.
        */

        QHash<int, QByteArray> roleNames;
        roleNames[Qt::UserRole] = "display";
        return roleNames;
    }

    void reset()
    {
        beginResetModel();
        endResetModel();
    }

    int filteredIndex(int actualIndex) const;
    int actualIndex(int filteredIndex);
    void setBackendModel(QStandardItemModel *model);

private:
    static Items filterItems(const Items &items, const QString &kind);

private:
    QStandardItemModel *m_backendModel;
    Items m_items, m_filteredItems;
    int m_count = -1;
    QHash<int, QByteArray> m_roles;
};

} // namespace StudioWelcome
