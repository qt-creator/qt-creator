/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QtGui/QDesktopServices>

#include <algorithm>

namespace Qt4ProjectManager {
namespace Internal {

namespace {
    const QLatin1String SettingsGroup("MaemoDeviceConfigs");
    const QLatin1String IdCounterKey("IdCounter");
    const QLatin1String ConfigListKey("ConfigList");
    const QLatin1String NameKey("Name");
    const QLatin1String TypeKey("Type");
    const QLatin1String HostKey("Host");
    const QLatin1String PortKey("Port");
    const QLatin1String UserNameKey("Uname");
    const QLatin1String AuthKey("Authentication");
    const QLatin1String KeyFileKey("KeyFile");
    const QLatin1String PasswordKey("Password");
    const QLatin1String TimeoutKey("Timeout");
    const QLatin1String InternalIdKey("InternalId");

    const QString DefaultKeyFile =
        QDesktopServices::storageLocation(QDesktopServices::HomeLocation)
        + QLatin1String("/.ssh/id_rsa");
};

class DevConfIdMatcher
{
public:
    DevConfIdMatcher(quint64 id) : m_id(id) {}
    bool operator()(const MaemoDeviceConfigurations::DeviceConfig &devConfig)
    {
        return devConfig.internalId == m_id;
    }
private:
    const quint64 m_id;
};

MaemoDeviceConfigurations::DeviceConfig::DeviceConfig(const QString &name)
    : name(name),
      type(Physical),
      port(22),
      authentication(Key),
      keyFile(DefaultKeyFile),
      timeout(30),
      internalId(MaemoDeviceConfigurations::instance().m_nextId++)
{
}

MaemoDeviceConfigurations::DeviceConfig::DeviceConfig(const QSettings &settings)
    : name(settings.value(NameKey).toString()),
      type(static_cast<DeviceType>(settings.value(TypeKey, Physical).toInt())),
      host(settings.value(HostKey).toString()),
      port(settings.value(PortKey, 22).toInt()),
      uname(settings.value(UserNameKey).toString()),
      authentication(static_cast<AuthType>(settings.value(AuthKey).toInt())),
      pwd(settings.value(PasswordKey).toString()),
      keyFile(settings.value(KeyFileKey).toString()),
      timeout(settings.value(TimeoutKey, 30).toInt()),
      internalId(settings.value(InternalIdKey, MaemoDeviceConfigurations::instance().m_nextId).toInt())
{
    if (internalId == MaemoDeviceConfigurations::instance().m_nextId)
        ++MaemoDeviceConfigurations::instance().m_nextId;
    qDebug("%s: name = %s, id = %llu", Q_FUNC_INFO, qPrintable(name), internalId);
}

MaemoDeviceConfigurations::DeviceConfig::DeviceConfig()
    : internalId(InvalidId)
{
}

bool MaemoDeviceConfigurations::DeviceConfig::isValid() const
{
    return internalId != InvalidId;
}

void MaemoDeviceConfigurations::DeviceConfig::save(QSettings &settings) const
{
    settings.setValue(NameKey, name);
    settings.setValue(TypeKey, type);
    settings.setValue(HostKey, host);
    settings.setValue(PortKey, port);
    settings.setValue(UserNameKey, uname);
    settings.setValue(AuthKey, authentication);
    settings.setValue(PasswordKey, pwd);
    settings.setValue(KeyFileKey, keyFile);
    settings.setValue(TimeoutKey, timeout);
    settings.setValue(InternalIdKey, internalId);
}

void MaemoDeviceConfigurations::setDevConfigs(const QList<DeviceConfig> &devConfigs)
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
    qDebug("Writing next id: %llu", m_nextId);
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
    qDebug("Read next id: %llu", m_nextId);
    int count = settings->beginReadArray(ConfigListKey);
    for (int i = 0; i < count; ++i) {
        settings->setArrayIndex(i);
        m_devConfigs.append(DeviceConfig(*settings));
    }
    settings->endArray();
    settings->endGroup();
}

MaemoDeviceConfigurations::DeviceConfig MaemoDeviceConfigurations::find(const QString &name) const
{
    qDebug("%s: Looking for name %s", Q_FUNC_INFO, qPrintable(name));
    QList<DeviceConfig>::ConstIterator resultIt =
        std::find_if(m_devConfigs.constBegin(), m_devConfigs.constEnd(),
                     DevConfNameMatcher(name));
    qDebug("Found: %d", resultIt != m_devConfigs.constEnd());
    return resultIt == m_devConfigs.constEnd() ? DeviceConfig() : *resultIt;
}

MaemoDeviceConfigurations::DeviceConfig MaemoDeviceConfigurations::find(int id) const
{
    qDebug("%s: Looking for id %d", Q_FUNC_INFO, id);
    QList<DeviceConfig>::ConstIterator resultIt =
        std::find_if(m_devConfigs.constBegin(), m_devConfigs.constEnd(),
                     DevConfIdMatcher(id));
    qDebug("Found: %d", resultIt != m_devConfigs.constEnd());
    return resultIt == m_devConfigs.constEnd() ? DeviceConfig() : *resultIt;
}

MaemoDeviceConfigurations *MaemoDeviceConfigurations::m_instance = 0;

} // namespace Internal
} // namespace Qt4ProjectManager
