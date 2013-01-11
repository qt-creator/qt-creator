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

#include "bardescriptorpermissionsmodel.h"

#include <QStringList>

using namespace Qnx;
using namespace Qnx::Internal;

BarDescriptorPermissionsModel::BarDescriptorPermissionsModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    initModel();
}

Qt::ItemFlags BarDescriptorPermissionsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    flags |= Qt::ItemIsUserCheckable;
    return flags;
}

int BarDescriptorPermissionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_permissions.size();
}

int BarDescriptorPermissionsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant BarDescriptorPermissionsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_permissions.size() || index.column() >= 1)
        return QVariant();

    BarDescriptorPermission perm = m_permissions[index.row()];
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return perm.permission;
    case Qt::CheckStateRole:
        return perm.checked ? Qt::Checked : Qt::Unchecked;
    case Qt::ToolTipRole:
        return perm.description;
    case IdentifierRole:
        return perm.identifier;
    }

    return QVariant();
}

bool BarDescriptorPermissionsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_permissions.size() || index.column() >= 1)
        return false;

    if (role == Qt::CheckStateRole) {
        BarDescriptorPermission &perm = m_permissions[index.row()];
        perm.checked = static_cast<Qt::CheckState>(value.toInt()) == Qt::Checked;
        emit dataChanged(index, index);

        return true;
    }

    return false;
}

QVariant BarDescriptorPermissionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation == Qt::Vertical)
        return QVariant();

    if (section == 0)
        return tr("Permission");

    return QVariant();
}

void BarDescriptorPermissionsModel::uncheckAll()
{
    setCheckStateAll(Qt::Unchecked);
}

void BarDescriptorPermissionsModel::checkAll()
{
    setCheckStateAll(Qt::Checked);
}

void BarDescriptorPermissionsModel::checkPermission(const QString &identifier)
{
    for (int i = 0; i < rowCount(); ++i) {
        QModelIndex idx = index(i, 0);
        if (data(idx, IdentifierRole).toString() == identifier)
            setData(idx, Qt::Checked, Qt::CheckStateRole);
    }
}

QStringList BarDescriptorPermissionsModel::checkedIdentifiers() const
{
    QStringList result;
    foreach (const BarDescriptorPermission &perm, m_permissions) {
        if (perm.checked)
            result << perm.identifier;
    }
    return result;
}

void BarDescriptorPermissionsModel::initModel()
{
    beginResetModel();
    m_permissions << BarDescriptorPermission(tr("Files"), QLatin1String("access_shared"),
                                             tr("Read and write files that are shared between all applications run by the current user."));
    m_permissions << BarDescriptorPermission(tr("Microphone"), QLatin1String("record_audio"),
                                             tr("Access the audio stream from the microphone."));
    m_permissions << BarDescriptorPermission(tr("GPS Location"), QLatin1String("read_geolocation"),
                                             tr("Read the current location of the device."));
    m_permissions << BarDescriptorPermission(tr("Camera"), QLatin1String("use_camera"),
                                             tr("Capture images and video using the cameras."));
    m_permissions << BarDescriptorPermission(tr("Internet"), QLatin1String("access_internet"),
                                             tr("Use a Wi-Fi, wired, or other connection to a destination that is not local."));
    m_permissions << BarDescriptorPermission(tr("Play Sounds"), QLatin1String("play_audio"),
                                             tr("Play an audio stream."));
    m_permissions << BarDescriptorPermission(tr("Post Notifications"), QLatin1String("post_notification"),
                                             tr("Post a notification to the notifications area of the screen."));
    m_permissions << BarDescriptorPermission(tr("Set Audio Volume"), QLatin1String("set_audio_volume"),
                                             tr("Change the volume of an audio stream being played."));
    m_permissions << BarDescriptorPermission(tr("Device Identifying Information"), QLatin1String("read_device_identifying_information"),
                                             tr("Access unique device identifying information (e.g. PIN)."));
    endResetModel();
}


void BarDescriptorPermissionsModel::setCheckStateAll(Qt::CheckState checkState)
{
    for (int i = 0; i < rowCount(); ++i)
        setData(index(i, 0), checkState, Qt::CheckStateRole);
}
