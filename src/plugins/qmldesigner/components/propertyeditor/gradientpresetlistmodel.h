// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QtQml/qqml.h>

class GradientPresetItem;

class GradientPresetListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit GradientPresetListModel(QObject *parent = nullptr);
    ~GradientPresetListModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void clearItems();
    void addItem(const GradientPresetItem &element);

    const QList<GradientPresetItem> &items() const;

    void sortItems();

    static void registerDeclarativeType();

protected:
    QList<GradientPresetItem> m_items;
    QHash<int, QByteArray> m_roleNames;
};

//QML_DECLARE_TYPE(GradientPresetListModel)
