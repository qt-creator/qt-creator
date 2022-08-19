// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "androidservicewidget.h"

#include <QAbstractTableModel>

namespace Android {
namespace Internal {

class AndroidServiceWidget::AndroidServiceModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    void setServices(const QList<AndroidServiceData> &androidServices);
    const QList<AndroidServiceData> &services();
    void addService();
    void removeService(int row);
    void servicesSaved();
signals:
    void validDataChanged();
    void invalidDataChanged();
private:
    int rowCount(const QModelIndex &/*parent*/ = QModelIndex()) const override;
    int columnCount(const QModelIndex &/*parent*/ = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
private:
    QList<AndroidServiceData> m_services;
};

} // namespace Internal
} // namespace Android
