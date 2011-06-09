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

#include "linuxdeviceconfigurations.h"
#include "maemoglobal.h"

#include <coreplugin/icore.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>
#include <QtCore/QStringBuilder>

#include <algorithm>

namespace RemoteLinux {
namespace Internal {

namespace {
    const QLatin1String SettingsGroup("MaemoDeviceConfigs");
    const QLatin1String IdCounterKey("IdCounter");
    const QLatin1String ConfigListKey("ConfigList");
    const QLatin1String DefaultKeyFilePathKey("DefaultKeyFile");
}

class DevConfNameMatcher
{
public:
    DevConfNameMatcher(const QString &name) : m_name(name) {}
    bool operator()(const LinuxDeviceConfiguration::ConstPtr &devConfig)
    {
        return devConfig->name() == m_name;
    }
private:
    const QString m_name;
};


LinuxDeviceConfigurations *LinuxDeviceConfigurations::instance(QObject *parent)
{
    if (m_instance == 0) {
        m_instance = new LinuxDeviceConfigurations(parent);
        m_instance->load();
    }
    return m_instance;
}

void LinuxDeviceConfigurations::replaceInstance(const LinuxDeviceConfigurations *other)
{
    Q_ASSERT(m_instance);
    m_instance->beginResetModel();
    copy(other, m_instance, false);
    m_instance->save();
    m_instance->endResetModel();
    emit m_instance->updated();
}

LinuxDeviceConfigurations *LinuxDeviceConfigurations::cloneInstance()
{
    LinuxDeviceConfigurations * const other = new LinuxDeviceConfigurations(0);
    copy(m_instance, other, true);
    return other;
}

void LinuxDeviceConfigurations::copy(const LinuxDeviceConfigurations *source,
    LinuxDeviceConfigurations *target, bool deep)
{
    if (deep) {
        foreach (const LinuxDeviceConfiguration::ConstPtr &devConf, source->m_devConfigs)
            target->m_devConfigs << LinuxDeviceConfiguration::create(devConf);
    } else {
        target->m_devConfigs = source->m_devConfigs;
    }
    target->m_defaultSshKeyFilePath = source->m_defaultSshKeyFilePath;
    target->m_nextId = source->m_nextId;
}

void LinuxDeviceConfigurations::save()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(SettingsGroup);
    settings->setValue(IdCounterKey, m_nextId);
    settings->setValue(DefaultKeyFilePathKey, m_defaultSshKeyFilePath);
    settings->beginWriteArray(ConfigListKey, m_devConfigs.count());
    for (int i = 0; i < m_devConfigs.count(); ++i) {
        settings->setArrayIndex(i);
        m_devConfigs.at(i)->save(*settings);
    }
    settings->endArray();
    settings->endGroup();
}

void LinuxDeviceConfigurations::addConfiguration(const LinuxDeviceConfiguration::Ptr &devConfig)
{
    devConfig->m_internalId = m_nextId++;
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    if (!defaultDeviceConfig(devConfig->osType()))
        devConfig->m_isDefault = true;
    m_devConfigs << devConfig;
    endInsertRows();
}

void LinuxDeviceConfigurations::removeConfiguration(int idx)
{
    Q_ASSERT(idx >= 0 && idx < rowCount());
    beginRemoveRows(QModelIndex(), idx, idx);
    const bool wasDefault = deviceAt(idx)->m_isDefault;
    const QString osType = deviceAt(idx)->osType();
    m_devConfigs.removeAt(idx);
    endRemoveRows();
    if (wasDefault) {
        for (int i = 0; i < m_devConfigs.count(); ++i) {
            if (deviceAt(i)->osType() == osType) {
                m_devConfigs.at(i)->m_isDefault = true;
                const QModelIndex changedIndex = index(i, 0);
                emit dataChanged(changedIndex, changedIndex);
                break;
            }
        }
    }
}

void LinuxDeviceConfigurations::setConfigurationName(int i, const QString &name)
{
    Q_ASSERT(i >= 0 && i < rowCount());
    m_devConfigs.at(i)->m_name = name;
    const QModelIndex changedIndex = index(i, 0);
    emit dataChanged(changedIndex, changedIndex);
}

void LinuxDeviceConfigurations::setSshParameters(int i,
    const Utils::SshConnectionParameters &params)
{
    Q_ASSERT(i >= 0 && i < rowCount());
    m_devConfigs.at(i)->m_sshParameters = params;
}

void LinuxDeviceConfigurations::setPortsSpec(int i, const QString &portsSpec)
{
    Q_ASSERT(i >= 0 && i < rowCount());
    m_devConfigs.at(i)->m_portsSpec = portsSpec;
}

void LinuxDeviceConfigurations::setDefaultDevice(int idx)
{
    Q_ASSERT(idx >= 0 && idx < rowCount());
    const LinuxDeviceConfiguration::Ptr &devConf = m_devConfigs.at(idx);
    if (devConf->m_isDefault)
        return;
    QModelIndex oldDefaultIndex;
    for (int i = 0; i < m_devConfigs.count(); ++i) {
        const LinuxDeviceConfiguration::Ptr &oldDefaultDev = m_devConfigs.at(i);
        if (oldDefaultDev->m_isDefault
                && oldDefaultDev->osType() == devConf->osType()) {
            oldDefaultDev->m_isDefault = false;
            oldDefaultIndex = index(i, 0);
            break;
        }
    }
    Q_ASSERT(oldDefaultIndex.isValid());
    emit dataChanged(oldDefaultIndex, oldDefaultIndex);
    devConf->m_isDefault = true;
    const QModelIndex newDefaultIndex = index(idx, 0);
    emit dataChanged(newDefaultIndex, newDefaultIndex);
}

LinuxDeviceConfigurations::LinuxDeviceConfigurations(QObject *parent)
    : QAbstractListModel(parent)
{
}

void LinuxDeviceConfigurations::load()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(SettingsGroup);
    m_nextId = settings->value(IdCounterKey, 1).toULongLong();
    m_defaultSshKeyFilePath = settings->value(DefaultKeyFilePathKey,
        LinuxDeviceConfiguration::defaultPrivateKeyFilePath()).toString();
    int count = settings->beginReadArray(ConfigListKey);
    for (int i = 0; i < count; ++i) {
        settings->setArrayIndex(i);
        LinuxDeviceConfiguration::Ptr devConf
            = LinuxDeviceConfiguration::create(*settings, m_nextId);
        m_devConfigs << devConf;
    }
    settings->endArray();
    settings->endGroup();
    ensureDefaultExists(LinuxDeviceConfiguration::Maemo5OsType);
    ensureDefaultExists(LinuxDeviceConfiguration::HarmattanOsType);
    ensureDefaultExists(LinuxDeviceConfiguration::MeeGoOsType);
    ensureDefaultExists(LinuxDeviceConfiguration::GenericLinuxOsType);
}

LinuxDeviceConfiguration::ConstPtr LinuxDeviceConfigurations::deviceAt(int idx) const
{
    Q_ASSERT(idx >= 0 && idx < rowCount());
    return m_devConfigs.at(idx);
}

bool LinuxDeviceConfigurations::hasConfig(const QString &name) const
{
    QList<LinuxDeviceConfiguration::Ptr>::ConstIterator resultIt =
        std::find_if(m_devConfigs.constBegin(), m_devConfigs.constEnd(),
            DevConfNameMatcher(name));
    return resultIt != m_devConfigs.constEnd();
}

LinuxDeviceConfiguration::ConstPtr LinuxDeviceConfigurations::find(LinuxDeviceConfiguration::Id id) const
{
    const int index = indexForInternalId(id);
    return index == -1 ? LinuxDeviceConfiguration::ConstPtr() : deviceAt(index);
}

LinuxDeviceConfiguration::ConstPtr LinuxDeviceConfigurations::defaultDeviceConfig(const QString &osType) const
{
    foreach (const LinuxDeviceConfiguration::ConstPtr &devConf, m_devConfigs) {
        if (devConf->m_isDefault && devConf->osType() == osType)
            return devConf;
    }
    return LinuxDeviceConfiguration::ConstPtr();
}

int LinuxDeviceConfigurations::indexForInternalId(LinuxDeviceConfiguration::Id internalId) const
{
    for (int i = 0; i < m_devConfigs.count(); ++i) {
        if (deviceAt(i)->m_internalId == internalId)
            return i;
    }
    return -1;
}

LinuxDeviceConfiguration::Id LinuxDeviceConfigurations::internalId(LinuxDeviceConfiguration::ConstPtr devConf) const
{
    return devConf ? devConf->m_internalId : LinuxDeviceConfiguration::InvalidId;
}

void LinuxDeviceConfigurations::ensureDefaultExists(const QString &osType)
{
    if (!defaultDeviceConfig(osType)) {
        foreach (const LinuxDeviceConfiguration::Ptr &devConf, m_devConfigs) {
            if (devConf->osType() == osType) {
                devConf->m_isDefault = true;
                break;
            }
        }
    }
}

int LinuxDeviceConfigurations::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_devConfigs.count();
}

QVariant LinuxDeviceConfigurations::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || role != Qt::DisplayRole)
        return QVariant();
    const LinuxDeviceConfiguration::ConstPtr devConf = deviceAt(index.row());
    QString name = devConf->name();
    if (devConf->m_isDefault) {
        name += QLatin1Char(' ') + tr("(default for %1)")
            .arg(MaemoGlobal::osTypeToString(devConf->osType()));
    }
    return name;
}

LinuxDeviceConfigurations *LinuxDeviceConfigurations::m_instance = 0;

} // namespace Internal
} // namespace RemoteLinux
