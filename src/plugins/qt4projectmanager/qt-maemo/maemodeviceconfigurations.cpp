/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "maemodeviceconfigurations.h"

#include <coreplugin/icore.h>

#include <QtCore/QSettings>
#include <QtCore/QStringBuilder>
#include <QtGui/QDesktopServices>

#include <algorithm>

typedef Core::SshServerInfo::AuthType AuthType;

namespace Qt4ProjectManager {
namespace Internal {

QString homeDirOnDevice(const QString &uname)
{
    return uname == QLatin1String("root")
        ? QString::fromLatin1("/root")
        : QLatin1String("/home/") + uname;
}

namespace {
    const QLatin1String SettingsGroup("MaemoDeviceConfigs");
    const QLatin1String IdCounterKey("IdCounter");
    const QLatin1String ConfigListKey("ConfigList");
    const QLatin1String NameKey("Name");
    const QLatin1String TypeKey("Type");
    const QLatin1String HostKey("Host");
    const QLatin1String SshPortKey("SshPort");
    const QLatin1String GdbServerPortKey("GdbServerPort");
    const QLatin1String UserNameKey("Uname");
    const QLatin1String AuthKey("Authentication");
    const QLatin1String KeyFileKey("KeyFile");
    const QLatin1String PasswordKey("Password");
    const QLatin1String TimeoutKey("Timeout");
    const QLatin1String InternalIdKey("InternalId");

    const QString DefaultKeyFile =
        QDesktopServices::storageLocation(QDesktopServices::HomeLocation)
        + QLatin1String("/.ssh/id_rsa");
    const int DefaultSshPortHW(22);
    const int DefaultSshPortSim(6666);
    const int DefaultGdbServerPortHW(10000);
    const int DefaultGdbServerPortSim(13219);
    const QString DefaultHostNameHW(QLatin1String("192.168.2.15"));
    const QString DefaultHostNameSim(QLatin1String("localhost"));
    const QString DefaultUserName(QLatin1String("developer"));
    const AuthType DefaultAuthType(Core::SshServerInfo::AuthByKey);
    const int DefaultTimeout(30);
    const MaemoDeviceConfig::DeviceType DefaultDeviceType(MaemoDeviceConfig::Physical);
};

class DevConfIdMatcher
{
public:
    DevConfIdMatcher(quint64 id) : m_id(id) {}
    bool operator()(const MaemoDeviceConfig &devConfig)
    {
        return devConfig.internalId == m_id;
    }
private:
    const quint64 m_id;
};

MaemoDeviceConfig::MaemoDeviceConfig(const QString &name, MaemoDeviceConfig::DeviceType devType)
    : name(name),
      type(devType),
      gdbServerPort(defaultGdbServerPort(type)),
      internalId(MaemoDeviceConfigurations::instance().m_nextId++)
{
    server.host = defaultHost(type);
    server.port = defaultSshPort(type);
    server.uname = DefaultUserName;
    server.authType = DefaultAuthType;
    server.privateKeyFile = DefaultKeyFile;
    server.timeout = DefaultTimeout;
}

MaemoDeviceConfig::MaemoDeviceConfig(const QSettings &settings,
                                     quint64 &nextId)
    : name(settings.value(NameKey).toString()),
      type(static_cast<DeviceType>(settings.value(TypeKey, DefaultDeviceType).toInt())),
      gdbServerPort(settings.value(GdbServerPortKey, defaultGdbServerPort(type)).toInt()),
      internalId(settings.value(InternalIdKey, nextId).toULongLong())
{
    if (internalId == nextId)
        ++nextId;
    server.host = settings.value(HostKey, defaultHost(type)).toString();
    server.port = settings.value(SshPortKey, defaultSshPort(type)).toInt();
    server.uname = settings.value(UserNameKey, DefaultUserName).toString();
    server.authType
        = static_cast<AuthType>(settings.value(AuthKey, DefaultAuthType).toInt());
    server.pwd = settings.value(PasswordKey).toString();
    server.privateKeyFile
        = settings.value(KeyFileKey, DefaultKeyFile).toString();
    server.timeout = settings.value(TimeoutKey, DefaultTimeout).toInt();
}

MaemoDeviceConfig::MaemoDeviceConfig()
    : internalId(InvalidId)
{
}

int MaemoDeviceConfig::defaultSshPort(DeviceType type) const
{
    return type == Physical ? DefaultSshPortHW : DefaultSshPortSim;
}

int MaemoDeviceConfig::defaultGdbServerPort(DeviceType type) const
{
    return type == Physical ? DefaultGdbServerPortHW : DefaultGdbServerPortSim;
}

QString MaemoDeviceConfig::defaultHost(DeviceType type) const
{
    return type == Physical ? DefaultHostNameHW : DefaultHostNameSim;
}

bool MaemoDeviceConfig::isValid() const
{
    return internalId != InvalidId;
}

void MaemoDeviceConfig::save(QSettings &settings) const
{
    settings.setValue(NameKey, name);
    settings.setValue(TypeKey, type);
    settings.setValue(HostKey, server.host);
    settings.setValue(SshPortKey, server.port);
    settings.setValue(GdbServerPortKey, gdbServerPort);
    settings.setValue(UserNameKey, server.uname);
    settings.setValue(AuthKey, server.authType);
    settings.setValue(PasswordKey, server.pwd);
    settings.setValue(KeyFileKey, server.privateKeyFile);
    settings.setValue(TimeoutKey, server.timeout);
    settings.setValue(InternalIdKey, internalId);
}

void MaemoDeviceConfigurations::setDevConfigs(const QList<MaemoDeviceConfig> &devConfigs)
{
    m_devConfigs = devConfigs;
    save();
    emit updated();
}

MaemoDeviceConfigurations &MaemoDeviceConfigurations::instance(QObject *parent)
{
    if (m_instance == 0)
        m_instance = new MaemoDeviceConfigurations(parent);
    return *m_instance;
}

void MaemoDeviceConfigurations::save()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(SettingsGroup);
    settings->setValue(IdCounterKey, m_nextId);
    settings->beginWriteArray(ConfigListKey, m_devConfigs.count());
    for (int i = 0; i < m_devConfigs.count(); ++i) {
        settings->setArrayIndex(i);
        m_devConfigs.at(i).save(*settings);
    }
    settings->endArray();
    settings->endGroup();
}

MaemoDeviceConfigurations::MaemoDeviceConfigurations(QObject *parent)
    : QObject(parent)
{
    load();
}

void MaemoDeviceConfigurations::load()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(SettingsGroup);
    m_nextId = settings->value(IdCounterKey, 1).toULongLong();
    int count = settings->beginReadArray(ConfigListKey);
    for (int i = 0; i < count; ++i) {
        settings->setArrayIndex(i);
        m_devConfigs.append(MaemoDeviceConfig(*settings, m_nextId));
    }
    settings->endArray();
    settings->endGroup();
}

MaemoDeviceConfig MaemoDeviceConfigurations::find(const QString &name) const
{
    QList<MaemoDeviceConfig>::ConstIterator resultIt =
        std::find_if(m_devConfigs.constBegin(), m_devConfigs.constEnd(),
                     DevConfNameMatcher(name));
    return resultIt == m_devConfigs.constEnd() ? MaemoDeviceConfig() : *resultIt;
}

MaemoDeviceConfig MaemoDeviceConfigurations::find(quint64 id) const
{
    QList<MaemoDeviceConfig>::ConstIterator resultIt =
        std::find_if(m_devConfigs.constBegin(), m_devConfigs.constEnd(),
                     DevConfIdMatcher(id));
    return resultIt == m_devConfigs.constEnd() ? MaemoDeviceConfig() : *resultIt;
}

MaemoDeviceConfigurations *MaemoDeviceConfigurations::m_instance = 0;

} // namespace Internal
} // namespace Qt4ProjectManager
