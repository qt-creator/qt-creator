/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#ifndef LINUXDEVICECONFIGURATIONS_H
#define LINUXDEVICECONFIGURATIONS_H

#include "linuxdeviceconfiguration.h"
#include "remotelinux_export.h"

#include <QtCore/QAbstractListModel>

QT_FORWARD_DECLARE_CLASS(QString)

namespace RemoteLinux {
namespace Internal {
class LinuxDeviceConfigurationsPrivate;
class LinuxDeviceConfigurationsSettingsWidget;
} // namespace Internal

class REMOTELINUX_EXPORT LinuxDeviceConfigurations : public QAbstractListModel
{
    Q_OBJECT
    friend class Internal::LinuxDeviceConfigurationsSettingsWidget;
public:
    ~LinuxDeviceConfigurations();

    static LinuxDeviceConfigurations *instance(QObject *parent = 0);

    // If you want to edit the list of device configurations programatically
    // (e.g. when doing device auto-detection), call cloneInstance() to get a copy,
    // do the changes there and then call replaceInstance() to write them back.
    // Callers must be prepared to deal with cloneInstance() temporarily returning null,
    // which happens if the settings dialog is currently open.
    // All other callers are required to do the clone/replace operation synchronously!
    static void replaceInstance(const LinuxDeviceConfigurations *other);
    static LinuxDeviceConfigurations *cloneInstance();

    LinuxDeviceConfiguration::ConstPtr deviceAt(int index) const;
    LinuxDeviceConfiguration::ConstPtr find(LinuxDeviceConfiguration::Id id) const;
    LinuxDeviceConfiguration::ConstPtr defaultDeviceConfig(const QString &osType) const;
    bool hasConfig(const QString &name) const;
    int indexForInternalId(LinuxDeviceConfiguration::Id internalId) const;
    LinuxDeviceConfiguration::Id internalId(LinuxDeviceConfiguration::ConstPtr devConf) const;

    void setDefaultSshKeyFilePath(const QString &path);
    QString defaultSshKeyFilePath() const;

    void addConfiguration(const LinuxDeviceConfiguration::Ptr &devConfig);
    void removeConfiguration(int index);
    void setConfigurationName(int i, const QString &name);
    void setSshParameters(int i, const Utils::SshConnectionParameters &params);
    void setFreePorts(int i, const PortList &freePorts);
    void setDefaultDevice(int index);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
        int role = Qt::DisplayRole) const;

signals:
    void updated();
    void cloningPossible();

private:
    LinuxDeviceConfigurations(QObject *parent);

    static void blockCloning();
    static void unblockCloning();

    void load();
    void save();
    static void copy(const LinuxDeviceConfigurations *source,
        LinuxDeviceConfigurations *target, bool deep);
    void ensureOneDefaultConfigurationPerOsType();

    Internal::LinuxDeviceConfigurationsPrivate * const d;
};

} // namespace RemoteLinux

#endif // LINUXDEVICECONFIGURATIONS_H
