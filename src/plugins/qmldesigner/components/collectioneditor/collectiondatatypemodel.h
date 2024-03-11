// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "collectiondetails.h"

#include <QAbstractListModel>
#include <QList>

namespace QmlDesigner {

class CollectionDataTypeModel : public QAbstractListModel
{
    Q_OBJECT

public:
    using DataType = CollectionDetails::DataType;
    CollectionDataTypeModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    static Q_INVOKABLE QString dataTypeToString(DataType dataType);
    static Q_INVOKABLE DataType dataTypeFromString(const QString &dataType);

    static void registerDeclarativeType();

private:
    struct Details;
    static const QList<Details> m_orderedDetails;
};

} // namespace QmlDesigner
