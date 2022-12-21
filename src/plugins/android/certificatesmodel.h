// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QPair>

namespace Android {
namespace Internal {

class CertificatesModel: public QAbstractListModel
{
public:
    CertificatesModel(const QString &rowCertificates, QObject *parent);

protected:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    QVector<QPair<QString, QString> > m_certs;
};

}
}
