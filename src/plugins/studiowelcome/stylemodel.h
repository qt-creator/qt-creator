/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#pragma once

#include <QAbstractListModel>
#include <QStandardItemModel>

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

        return "";
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

    int filteredIndex(int actualIndex);
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

