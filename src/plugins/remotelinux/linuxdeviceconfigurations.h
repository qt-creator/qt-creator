/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef MAEMODEVICECONFIGURATIONS_H
#define MAEMODEVICECONFIGURATIONS_H

#include "linuxdeviceconfiguration.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>


namespace RemoteLinux {
namespace Internal {

class LinuxDeviceConfigurations : public QAbstractListModel
{
    Q_OBJECT
    Q_DISABLE_COPY(LinuxDeviceConfigurations)
    friend class MaemoDeviceConfigurationsSettingsWidget;
public:
    static LinuxDeviceConfigurations *instance(QObject *parent = 0);

    static void replaceInstance(const LinuxDeviceConfigurations *other);
    static LinuxDeviceConfigurations *cloneInstance();

    LinuxDeviceConfiguration::ConstPtr deviceAt(int index) const;
    LinuxDeviceConfiguration::ConstPtr find(LinuxDeviceConfiguration::Id id) const;
    LinuxDeviceConfiguration::ConstPtr defaultDeviceConfig(const QString &osType) const;
    bool hasConfig(const QString &name) const;
    int indexForInternalId(LinuxDeviceConfiguration::Id internalId) const;
    LinuxDeviceConfiguration::Id internalId(LinuxDeviceConfiguration::ConstPtr devConf) const;

    void setDefaultSshKeyFilePath(const QString &path) { m_defaultSshKeyFilePath = path; }
    QString defaultSshKeyFilePath() const { return m_defaultSshKeyFilePath; }

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

private:
    LinuxDeviceConfigurations(QObject *parent);
    void load();
    void save();
    static void copy(const LinuxDeviceConfigurations *source,
        LinuxDeviceConfigurations *target, bool deep);
    void addConfiguration(const LinuxDeviceConfiguration::Ptr &devConfig);
    void ensureDefaultExists(const QString &osType);

    static LinuxDeviceConfigurations *m_instance;
    LinuxDeviceConfiguration::Id m_nextId;
    QList<LinuxDeviceConfiguration::Ptr> m_devConfigs;
    QString m_defaultSshKeyFilePath;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMODEVICECONFIGURATIONS_H
