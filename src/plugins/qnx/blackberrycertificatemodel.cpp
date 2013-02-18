/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberrycertificatemodel.h"
#include "blackberrycertificate.h"
#include "blackberryconfiguration.h"

#include <coreplugin/icore.h>

#include <QSettings>
#include <QStringList>

namespace Qnx {
namespace Internal {

const QLatin1String SettingsGroup("BlackBerryConfiguration");
const QLatin1String CertificateGroup("Certificates");

BlackBerryCertificateModel::BlackBerryCertificateModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_activeCertificate(0)
{
    load();
}

BlackBerryCertificateModel::~BlackBerryCertificateModel()
{
}

int BlackBerryCertificateModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_certificates.count();
}

int BlackBerryCertificateModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return ColumnCount;
}

QVariant BlackBerryCertificateModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= rowCount() || index.column() >= columnCount())
        return QVariant();

    const BlackBerryCertificate *cert = m_certificates.at(index.row());

    if (role == Qt::CheckStateRole) {
        if (index.column() == CertActive)
            return (m_activeCertificate == cert) ? Qt::Checked : Qt::Unchecked;
    } else if (role == Qt::DisplayRole) {
        if (index.column() == CertPath)
            return cert->fileName();
        else if (index.column() == CertAuthor)
            return cert->author();
    }

    return QVariant();
}

QVariant BlackBerryCertificateModel::headerData(int section,
        Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Vertical)
        return section;

    switch (section) {
    case CertPath:
        return tr("Path");
    case CertAuthor:
        return tr("Author");
    case CertActive:
        return tr("Active");
    default:
        break;
    }

    return section;
}

bool BlackBerryCertificateModel::setData(const QModelIndex &ind,
        const QVariant &value, int role)
{
    Q_UNUSED(value);

    if (role == Qt::CheckStateRole && ind.column() == CertActive) {
        const int oldIndex = m_certificates.indexOf(m_activeCertificate);

        m_activeCertificate = m_certificates.at(ind.row());

        if (oldIndex >= 0)
            emit dataChanged(index(oldIndex, CertActive), index(oldIndex, CertActive));

        emit dataChanged(ind, ind);

        return true;
    }

    return false;
}

bool BlackBerryCertificateModel::removeRows(int row, int count,
        const QModelIndex &parent)
{

    beginRemoveRows(parent, row, row + count - 1);

    for (int i = 0; i < count; i++) {
        m_certificates.removeAt(row);
        //XXX shall we also delete from disk?
    }

    endRemoveRows();

    return true;
}

Qt::ItemFlags BlackBerryCertificateModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    switch (index.column()) {
    case CertActive:
        flags |= Qt::ItemIsUserCheckable;
        break;
    default:
        break;
    }

    return flags;
}

BlackBerryCertificate * BlackBerryCertificateModel::activeCertificate() const
{
    return m_activeCertificate;
}

QList<BlackBerryCertificate*> BlackBerryCertificateModel::certificates() const
{
    return m_certificates;
}

bool BlackBerryCertificateModel::insertCertificate(BlackBerryCertificate *certificate)
{
    if (m_certificates.contains(certificate))
        return false;

    beginInsertRows(QModelIndex(), m_certificates.count(), m_certificates.count());

    if (m_certificates.isEmpty())
        m_activeCertificate = certificate;

    certificate->setParent(this);
    m_certificates << certificate;

    endInsertRows();

    return true;
}

void BlackBerryCertificateModel::load()
{
    BlackBerryConfiguration &configuration = BlackBerryConfiguration::instance();
    m_certificates = configuration.certificates();
    m_activeCertificate = configuration.activeCertificate();
}

} // namespace Internal
} // namespace Qnx
