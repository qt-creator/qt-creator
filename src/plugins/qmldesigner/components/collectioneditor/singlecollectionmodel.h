// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractTableModel>
#include <QJsonObject>

namespace QmlDesigner {

class ModelNode;

class SingleCollectionModel : public QAbstractTableModel
{
    Q_OBJECT

    Q_PROPERTY(QString collectionName MEMBER m_collectionName NOTIFY collectionNameChanged)

public:
    enum class SourceFormat { Unknown, Json, Csv };
    explicit SingleCollectionModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void loadCollection(const ModelNode &sourceNode, const QString &collection);

signals:
    void collectionNameChanged(const QString &collectionName);

private:
    void setCollectionName(const QString &newCollectionName);
    void setCollectionSourceFormat(SourceFormat sourceFormat);
    void loadJsonCollection(const QString &source, const QString &collection);
    void loadCsvCollection(const QString &source, const QString &collectionName);

    QStringList m_headers;
    QList<QJsonObject> m_elements;
    QString m_collectionName;
    SourceFormat m_sourceFormat = SourceFormat::Unknown;
};

} // namespace QmlDesigner
