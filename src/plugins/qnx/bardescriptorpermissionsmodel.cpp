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
    m_permissions << BarDescriptorPermission(tr("BlackBerry Messenger"), QLatin1String("bbm_connect"),
                                             tr("<html><head/><body><p>Allows this app to connect to the BBM Social Platform to access BBM "
                                                "contact lists and user profiles, invite BBM contacts to download your "
                                                "app, initiate BBM chats and share content from within your app, or "
                                                "stream data between apps in real time.</p></body></html>"));
    m_permissions << BarDescriptorPermission(tr("Calendar"), QLatin1String("access_pimdomain_calendars"),
                                             tr("<html><head/><body><p>Allows this app to access the calendar on the device. This access "
                                                "includes viewing, adding, and deleting calendar appointments.</p></body></html>"));
    m_permissions << BarDescriptorPermission(tr("Camera"), QLatin1String("use_camera"),
                                             tr("<html><head/><body><p>Allows this app to take pictures, record video, and use the flash.</p></body></html>"));
    m_permissions << BarDescriptorPermission(tr("Contacts"), QLatin1String("access_pimdomain_contacts"),
                                             tr("<html><head/><body><p>Allows this app to access the contacts stored on the device. This "
                                                "access includes viewing, creating, and deleting the contacts.</p></body></html>"));
    m_permissions << BarDescriptorPermission(tr("Device Identifying Information"), QLatin1String("read_device_identifying_information"),
                                             tr("<html><head/><body><p>Allows this app to access device identifiers such as serial number and PIN.</p></body></html>"));
    m_permissions << BarDescriptorPermission(tr("Email and PIN Messages"), QLatin1String("access_pimdomain_messages"),
                                             tr("<html><head/><body><p>Allows this app to access the email and PIN messages stored on the "
                                                "device. This access includes viewing, creating, sending, and deleting the messages.</p></body></html>"));
    m_permissions << BarDescriptorPermission(tr("GPS Location"), QLatin1String("read_geolocation"),
                                             tr("<html><head/><body><p>Allows this app to access the current GPS location of the device.</p></body></html>"));
    m_permissions << BarDescriptorPermission(tr("Internet"), QLatin1String("access_internet"),
                                             tr("<html><head/><body><p>Allows this app to use Wi-fi, wired, or other connections to a "
                                                "destination that is not local on the user's device.</p></body></html>"));
    m_permissions << BarDescriptorPermission(tr("Location"), QLatin1String("access_location_services"),
                                             tr("<html><head/><body><p>Allows this app to access the device's current or saved locations.</p></body></html>"));
    m_permissions << BarDescriptorPermission(tr("Microphone"), QLatin1String("record_audio"),
                                             tr("<html><head/><body><p>Allows this app to record sound using the microphone.</p></body></html>"));
    m_permissions << BarDescriptorPermission(tr("Notebooks"), QLatin1String("access_pimdomain_notebooks"),
                                             tr("<html><head/><body><p>Allows this app to access the content stored in the notebooks on the "
                                                "device. This access includes adding and deleting entries and content.</p></body></html>"));
    m_permissions << BarDescriptorPermission(tr("Post Notifications"), QLatin1String("post_notification"),
                                             tr("<html><head/><body><p>Post a notification to the notifications area of the screen.</p></body></html>"));
    m_permissions << BarDescriptorPermission(tr("Push"), QLatin1String("_sys_use_consumer_push"),
                                             tr("<html><head/><body><p>Allows this app to use the Push Service with the BlackBerry Internet "
                                                "Service. This access allows the app to receive and request push "
                                                "messages. To use the Push Service with the BlackBerry Internet Service, "
                                                "you must register with BlackBerry. When you register, you "
                                                "receive a confirmation email message that contains information that "
                                                "your application needs to receive and request push messages. For more "
                                                "information about registering, visit "
                                                "https://developer.blackberry.com/services/push/. If you're using the "
                                                "Push Service with the BlackBerry Enterprise Server or the BlackBerry "
                                                "Device Service, you don't need to register with BlackBerry.</p></body></html>"));
    m_permissions << BarDescriptorPermission(tr("Run When Backgrounded"), QLatin1String("run_when_backgrounded"),
                                             tr("<html><head/><body><p>Allows background processing. Without this permission, the app is "
                                                "stopped when the user switches focus to another app. Apps that use this "
                                                "permission are rigorously reviewed for acceptance to BlackBerry App "
                                                "World storefront for their use of power.</p></body></html>"));
    m_permissions << BarDescriptorPermission(tr("Shared Files"), QLatin1String("access_shared"),
                                             tr("<html><head/><body><p>Allows this app to access pictures, music, documents, and other files "
                                                "stored on the user's device, at a remote storage provider, on a media "
                                                "card, or in the cloud.</p></body></html>"));
    m_permissions << BarDescriptorPermission(tr("Text Messages"), QLatin1String("access_sms_mms"),
                                             tr("<html><head/><body><p>Allows this app to access the text messages stored on the device. The "
                                                "access includes viewing, creating, sending, and deleting text messages.</p></body></html>"));
    endResetModel();
}


void BarDescriptorPermissionsModel::setCheckStateAll(Qt::CheckState checkState)
{
    for (int i = 0; i < rowCount(); ++i)
        setData(index(i, 0), checkState, Qt::CheckStateRole);
}
