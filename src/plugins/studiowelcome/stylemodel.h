// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QSortFilterProxyModel>
#include <QStandardItemModel>

namespace StudioWelcome {

class StyleModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit StyleModel(QObject *parent = nullptr);

    Q_INVOKABLE void filter(const QString &what = "all");
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE int findIndex(const QString &styleName) const;
    Q_INVOKABLE int findSourceIndex(const QString &styleName) const;

private:
    enum Roles{IconNameRole = Qt::UserRole + 1};
};

} // namespace StudioWelcome
