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
#include "linuxdeviceconfigurations.h"

#include "remotelinuxutils.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QSettings>
#include <QtCore/QString>

#include <algorithm>

namespace RemoteLinux {
namespace Internal {

namespace {
const QLatin1String SettingsGroup("MaemoDeviceConfigs");
const QLatin1String IdCounterKey("IdCounter");
const QLatin1String ConfigListKey("ConfigList");
const QLatin1String DefaultKeyFilePathKey("DefaultKeyFile");

bool cloningBlocked = false;

class DevConfNameMatcher
{
public:
    DevConfNameMatcher(const QString &name) : m_name(name) {}
    bool operator()(const LinuxDeviceConfiguration::ConstPtr &devConfig)
    {
        return devConfig->displayName() == m_name;
    }
private:
    const QString m_name;
};

} // anonymous namespace

class LinuxDeviceConfigurationsPrivate
{
public:
    static LinuxDeviceConfigurations *instance;
    LinuxDeviceConfiguration::Id nextId;
    QList<LinuxDeviceConfiguration::Ptr> devConfigs;
    QString defaultSshKeyFilePath;
};
LinuxDeviceConfigurations *LinuxDeviceConfigurationsPrivate::instance = 0;

} // namespace Internal

using namespace Internal;


LinuxDeviceConfigurations *LinuxDeviceConfigurations::instance(QObject *parent)
{
    if (LinuxDeviceConfigurationsPrivate::instance == 0) {
        LinuxDeviceConfigurationsPrivate::instance = new LinuxDeviceConfigurations(parent);
        LinuxDeviceConfigurationsPrivate::instance->load();
    }
    return LinuxDeviceConfigurationsPrivate::instance;
}

void LinuxDeviceConfigurations::replaceInstance(const LinuxDeviceConfigurations *other)
{
    Q_ASSERT(LinuxDeviceConfigurationsPrivate::instance);

    LinuxDeviceConfigurationsPrivate::instance->beginResetModel();
    copy(other, LinuxDeviceConfigurationsPrivate::instance, false);
    LinuxDeviceConfigurationsPrivate::instance->save();
    LinuxDeviceConfigurationsPrivate::instance->endResetModel();
    emit LinuxDeviceConfigurationsPrivate::instance->updated();
}

LinuxDeviceConfigurations *LinuxDeviceConfigurations::cloneInstance()
{
    if (cloningBlocked)
        return 0;
    LinuxDeviceConfigurations * const other = new LinuxDeviceConfigurations(0);
    copy(LinuxDeviceConfigurationsPrivate::instance, other, true);
    return other;
}

void LinuxDeviceConfigurations::copy(const LinuxDeviceConfigurations *source,
    LinuxDeviceConfigurations *target, bool deep)
{
    if (deep) {
        foreach (const LinuxDeviceConfiguration::ConstPtr &devConf, source->d->devConfigs)
            target->d->devConfigs << LinuxDeviceConfiguration::create(devConf);
    } else {
        target->d->devConfigs = source->d->devConfigs;
    }
    target->d->defaultSshKeyFilePath = source->d->defaultSshKeyFilePath;
    target->d->nextId = source->d->nextId;
}

void LinuxDeviceConfigurations::save()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(SettingsGroup);
    settings->setValue(IdCounterKey, d->nextId);
    settings->setValue(DefaultKeyFilePathKey, d->defaultSshKeyFilePath);
    settings->beginWriteArray(ConfigListKey);
    int skippedCount = 0;
    for (int i = 0; i < d->devConfigs.count(); ++i) {
        const LinuxDeviceConfiguration::ConstPtr &devConf = d->devConfigs.at(i);
        if (devConf->isAutoDetected()) {
            ++skippedCount;
        } else {
            settings->setArrayIndex(i-skippedCount);
            devConf->save(*settings);
        }
    }
    settings->endArray();
    settings->endGroup();
}

void LinuxDeviceConfigurations::addConfiguration(const LinuxDeviceConfiguration::Ptr &devConfig)
{
    QTC_ASSERT(this != LinuxDeviceConfigurationsPrivate::instance, return);

    // Ensure uniqueness of name.
    QString name = devConfig->displayName();
    if (hasConfig(name)) {
        const QString nameTemplate = name + QLatin1String(" (%1)");
        int suffix = 2;
        do
            name = nameTemplate.arg(QString::number(suffix++));
        while (hasConfig(name));
    }
    devConfig->setDisplayName(name);

    devConfig->setInternalId(d->nextId++);
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    if (!defaultDeviceConfig(devConfig->osType()))
        devConfig->setDefault(true);
    d->devConfigs << devConfig;
    endInsertRows();
}

void LinuxDeviceConfigurations::removeConfiguration(int idx)
{
    QTC_ASSERT(this != LinuxDeviceConfigurationsPrivate::instance, return);
    Q_ASSERT(idx >= 0 && idx < rowCount());

    beginRemoveRows(QModelIndex(), idx, idx);
    const bool wasDefault = deviceAt(idx)->isDefault();
    const QString osType = deviceAt(idx)->osType();
    d->devConfigs.removeAt(idx);
    endRemoveRows();
    if (wasDefault) {
        for (int i = 0; i < d->devConfigs.count(); ++i) {
            if (deviceAt(i)->osType() == osType) {
                d->devConfigs.at(i)->setDefault(true);
                const QModelIndex changedIndex = index(i, 0);
                emit dataChanged(changedIndex, changedIndex);
                break;
            }
        }
    }
}

void LinuxDeviceConfigurations::setDefaultSshKeyFilePath(const QString &path)
{
    QTC_ASSERT(this != LinuxDeviceConfigurationsPrivate::instance, return);

    d->defaultSshKeyFilePath = path;
}

QString LinuxDeviceConfigurations::defaultSshKeyFilePath() const
{
    return d->defaultSshKeyFilePath;
}

void LinuxDeviceConfigurations::setConfigurationName(int i, const QString &name)
{
    QTC_ASSERT(this != LinuxDeviceConfigurationsPrivate::instance, return);
    Q_ASSERT(i >= 0 && i < rowCount());

    d->devConfigs.at(i)->setDisplayName(name);
    const QModelIndex changedIndex = index(i, 0);
    emit dataChanged(changedIndex, changedIndex);
}

void LinuxDeviceConfigurations::setSshParameters(int i,
    const Utils::SshConnectionParameters &params)
{
    QTC_ASSERT(this != LinuxDeviceConfigurationsPrivate::instance, return);
    Q_ASSERT(i >= 0 && i < rowCount());

    d->devConfigs.at(i)->setSshParameters(params);
}

void LinuxDeviceConfigurations::setFreePorts(int i, const PortList &freePorts)
{
    QTC_ASSERT(this != LinuxDeviceConfigurationsPrivate::instance, return);
    Q_ASSERT(i >= 0 && i < rowCount());

    d->devConfigs.at(i)->setFreePorts(freePorts);
}

void LinuxDeviceConfigurations::setDefaultDevice(int idx)
{
    QTC_ASSERT(this != LinuxDeviceConfigurationsPrivate::instance, return);
    Q_ASSERT(idx >= 0 && idx < rowCount());

    const LinuxDeviceConfiguration::Ptr &devConf = d->devConfigs.at(idx);
    if (devConf->isDefault())
        return;
    QModelIndex oldDefaultIndex;
    for (int i = 0; i < d->devConfigs.count(); ++i) {
        const LinuxDeviceConfiguration::Ptr &oldDefaultDev = d->devConfigs.at(i);
        if (oldDefaultDev->isDefault() && oldDefaultDev->osType() == devConf->osType()) {
            oldDefaultDev->setDefault(false);
            oldDefaultIndex = index(i, 0);
            break;
        }
    }

    QTC_CHECK(oldDefaultIndex.isValid());
    emit dataChanged(oldDefaultIndex, oldDefaultIndex);
    devConf->setDefault(true);
    const QModelIndex newDefaultIndex = index(idx, 0);
    emit dataChanged(newDefaultIndex, newDefaultIndex);
}

LinuxDeviceConfigurations::LinuxDeviceConfigurations(QObject *parent)
    : QAbstractListModel(parent), d(new LinuxDeviceConfigurationsPrivate)
{
}

void LinuxDeviceConfigurations::blockCloning()
{
    QTC_ASSERT(!cloningBlocked, return);
    cloningBlocked = true;
}

void LinuxDeviceConfigurations::unblockCloning()
{
    QTC_ASSERT(cloningBlocked, return);
    cloningBlocked = false;
    emit instance()->cloningPossible();
}

LinuxDeviceConfigurations::~LinuxDeviceConfigurations()
{
    delete d;
}

void LinuxDeviceConfigurations::load()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(SettingsGroup);
    d->nextId = settings->value(IdCounterKey, 1).toULongLong();
    d->defaultSshKeyFilePath = settings->value(DefaultKeyFilePathKey,
        LinuxDeviceConfiguration::defaultPrivateKeyFilePath()).toString();
    int count = settings->beginReadArray(ConfigListKey);
    for (int i = 0; i < count; ++i) {
        settings->setArrayIndex(i);
        LinuxDeviceConfiguration::Ptr devConf
            = LinuxDeviceConfiguration::create(*settings, d->nextId);
        d->devConfigs << devConf;
    }
    settings->endArray();
    settings->endGroup();
    ensureOneDefaultConfigurationPerOsType();
}

LinuxDeviceConfiguration::ConstPtr LinuxDeviceConfigurations::deviceAt(int idx) const
{
    Q_ASSERT(idx >= 0 && idx < rowCount());
    return d->devConfigs.at(idx);
}

bool LinuxDeviceConfigurations::hasConfig(const QString &name) const
{
    QList<LinuxDeviceConfiguration::Ptr>::ConstIterator resultIt =
        std::find_if(d->devConfigs.constBegin(), d->devConfigs.constEnd(),
            DevConfNameMatcher(name));
    return resultIt != d->devConfigs.constEnd();
}

LinuxDeviceConfiguration::ConstPtr LinuxDeviceConfigurations::find(LinuxDeviceConfiguration::Id id) const
{
    const int index = indexForInternalId(id);
    return index == -1 ? LinuxDeviceConfiguration::ConstPtr() : deviceAt(index);
}

LinuxDeviceConfiguration::ConstPtr LinuxDeviceConfigurations::defaultDeviceConfig(const QString &osType) const
{
    foreach (const LinuxDeviceConfiguration::ConstPtr &devConf, d->devConfigs) {
        if (devConf->isDefault() && devConf->osType() == osType)
            return devConf;
    }
    return LinuxDeviceConfiguration::ConstPtr();
}

int LinuxDeviceConfigurations::indexForInternalId(LinuxDeviceConfiguration::Id internalId) const
{
    for (int i = 0; i < d->devConfigs.count(); ++i) {
        if (deviceAt(i)->internalId() == internalId)
            return i;
    }
    return -1;
}

LinuxDeviceConfiguration::Id LinuxDeviceConfigurations::internalId(LinuxDeviceConfiguration::ConstPtr devConf) const
{
    return devConf ? devConf->internalId() : LinuxDeviceConfiguration::InvalidId;
}

void LinuxDeviceConfigurations::ensureOneDefaultConfigurationPerOsType()
{
    QHash<QString, bool> osTypeHasDefault;

    // Step 1: Ensure there's at most one default configuration per device type.
    foreach (const LinuxDeviceConfiguration::Ptr &devConf, d->devConfigs) {
        if (devConf->isDefault()) {
            if (osTypeHasDefault.value(devConf->osType()))
                devConf->setDefault(false);
            else
                osTypeHasDefault.insert(devConf->osType(), true);
        }
    }

    // Step 2: Ensure there's at least one default configuration per device type.
    foreach (const LinuxDeviceConfiguration::Ptr &devConf, d->devConfigs) {
        if (!osTypeHasDefault.value(devConf->osType())) {
            devConf->setDefault(true);
            osTypeHasDefault.insert(devConf->osType(), true);
        }
    }
}

int LinuxDeviceConfigurations::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return d->devConfigs.count();
}

QVariant LinuxDeviceConfigurations::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || role != Qt::DisplayRole)
        return QVariant();
    const LinuxDeviceConfiguration::ConstPtr devConf = deviceAt(index.row());
    QString name = devConf->displayName();
    if (devConf->isDefault()) {
        name += QLatin1Char(' ') + tr("(default for %1)")
            .arg(RemoteLinuxUtils::osTypeToString(devConf->osType()));
    }
    return name;
}

} // namespace RemoteLinux
