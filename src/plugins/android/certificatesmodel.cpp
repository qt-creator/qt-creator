// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "certificatesmodel.h"

#include <QComboBox>

using namespace Android;
using namespace Android::Internal;

namespace {
const QLatin1String AliasString("Alias name:");
const QLatin1String CertificateSeparator("*******************************************");
}

CertificatesModel::CertificatesModel(const QString &rowCertificates, QObject *parent)
    : QAbstractListModel(parent)
{
    int from = rowCertificates.indexOf(AliasString);
    QPair<QString, QString> item;
    while (from > -1) {
        from += 11;// strlen(AliasString);
        const int eol = rowCertificates.indexOf(QLatin1Char('\n'), from);
        item.first = rowCertificates.mid(from, eol - from).trimmed();
        const int eoc = rowCertificates.indexOf(CertificateSeparator, eol);
        item.second = rowCertificates.mid(eol + 1, eoc - eol - 2).trimmed();
        from = rowCertificates.indexOf(AliasString, eoc);
        m_certs.push_back(item);
    }
}

int CertificatesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_certs.size();
}

QVariant CertificatesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::ToolTipRole))
        return {};
    if (role == Qt::DisplayRole)
        return m_certs[index.row()].first;
    return m_certs[index.row()].second;
}
