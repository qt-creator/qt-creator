// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <designsystem/dsstore.h>

#include <QObject>
class QAbstractItemModel;

namespace QmlDesigner {
class CollectionModel;
class DesignSystemInterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList collections READ collections NOTIFY loadFinished FINAL)

public:
    DesignSystemInterface(DSStore *store);
    ~DesignSystemInterface();

    Q_INVOKABLE void loadDesignSystem();
    Q_INVOKABLE QAbstractItemModel *model(const QString &typeName);

    QStringList collections() const;

signals:
    void loadFinished();

private:
    class DSStore *m_store = nullptr;
    std::map<QString, std::unique_ptr<CollectionModel>> m_models;
};

} // namespace QmlDesigner
