/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QHash>
#include <QList>
#include <QSettings>
#include <QString>
#include <QVariantHash>

#include <algorithm>

namespace RemoteLinux {
namespace Internal {

namespace {
const QLatin1String SettingsGroup("MaemoDeviceConfigs");
const QLatin1String IdCounterKey("IdCounter");
const QLatin1String ConfigListKey("ConfigList");
const QLatin1String DefaultKeyFilePathKey("DefaultKeyFile");
const char DefaultConfigsKey[] = "DefaultConfigs";

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
    static LinuxDeviceConfigurations *clonedInstance;
    LinuxDeviceConfiguration::Id nextId;
    QList<LinuxDeviceConfiguration::Ptr> devConfigs;
    QHash<QString, LinuxDeviceConfiguration::Id> defaultConfigs;
    QString defaultSshKeyFilePath;
};
LinuxDeviceConfigurations *LinuxDeviceConfigurationsPrivate::instance = 0;
LinuxDeviceConfigurations *LinuxDeviceConfigurationsPrivate::clonedInstance = 0;

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

void LinuxDeviceConfigurations::replaceInstance()
{
    Q_ASSERT(LinuxDeviceConfigurationsPrivate::instance);

    LinuxDeviceConfigurationsPrivate::instance->beginResetModel();
    copy(LinuxDeviceConfigurationsPrivate::clonedInstance,
        LinuxDeviceConfigurationsPrivate::instance, false);
    LinuxDeviceConfigurationsPrivate::instance->save();
    LinuxDeviceConfigurationsPrivate::instance->endResetModel();
    emit LinuxDeviceConfigurationsPrivate::instance->updated();
}

void LinuxDeviceConfigurations::removeClonedInstance()
{
    delete LinuxDeviceConfigurationsPrivate::clonedInstance;
    LinuxDeviceConfigurationsPrivate::clonedInstance = 0;
}

LinuxDeviceConfigurations *LinuxDeviceConfigurations::cloneInstance()
{
    QTC_ASSERT(!LinuxDeviceConfigurationsPrivate::clonedInstance, return 0);

    LinuxDeviceConfigurationsPrivate::clonedInstance
        = new LinuxDeviceConfigurations(LinuxDeviceConfigurationsPrivate::instance);
    copy(LinuxDeviceConfigurationsPrivate::instance,
        LinuxDeviceConfigurationsPrivate::clonedInstance, true);
    return LinuxDeviceConfigurationsPrivate::clonedInstance;
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
    target->d->defaultConfigs = source->d->defaultConfigs;
}

void LinuxDeviceConfigurations::save()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    settings->setValue(IdCounterKey, d->nextId);
    settings->setValue(DefaultKeyFilePathKey, d->defaultSshKeyFilePath);
    QVariantHash defaultDevsHash;
    for (QHash<QString, LinuxDeviceConfiguration::Id>::ConstIterator it = d->defaultConfigs.constBegin();
             it != d->defaultConfigs.constEnd(); ++it) {
        defaultDevsHash.insert(it.key(), it.value());
    }
    settings->setValue(QLatin1String(DefaultConfigsKey), defaultDevsHash);
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
    QTC_ASSERT(this != LinuxDeviceConfigurationsPrivate::instance
        || (devConfig->isAutoDetected()), return);

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

    if (this == LinuxDeviceConfigurations::instance())
        devConfig->setInternalId(d->nextId++);

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    if (!defaultDeviceConfig(devConfig->osType()))
        d->defaultConfigs.insert(devConfig->osType(), devConfig->internalId());
    d->devConfigs << devConfig;
    endInsertRows();
    if (this == d->instance && d->clonedInstance) {
        d->clonedInstance->addConfiguration(
            LinuxDeviceConfiguration::Ptr(new LinuxDeviceConfiguration(devConfig)));
    }
    emit updated();
}

void LinuxDeviceConfigurations::removeConfiguration(int idx)
{
    QTC_ASSERT(idx >= 0 && idx < rowCount(), return);
    const LinuxDeviceConfiguration::ConstPtr deviceConfig = deviceAt(idx);
    QTC_ASSERT(this != LinuxDeviceConfigurationsPrivate::instance
        || deviceConfig->isAutoDetected(), return);

    beginRemoveRows(QModelIndex(), idx, idx);
    const bool wasDefault
        = d->defaultConfigs.value(deviceConfig->osType()) == deviceConfig->internalId();
    const QString osType = deviceConfig->osType();
    d->devConfigs.removeAt(idx);
    endRemoveRows();
    if (wasDefault) {
        for (int i = 0; i < d->devConfigs.count(); ++i) {
            if (deviceAt(i)->osType() == osType) {
                d->defaultConfigs.insert(deviceAt(i)->osType(), deviceAt(i)->internalId());
                const QModelIndex changedIndex = index(i, 0);
                emit dataChanged(changedIndex, changedIndex);
                break;
            }
        }
    }
    if (this == d->instance && d->clonedInstance) {
        d->clonedInstance->removeConfiguration(d->clonedInstance->
            indexForInternalId(deviceConfig->internalId()));
    }
    emit updated();
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

void LinuxDeviceConfigurations::setDefaultDevice(int idx)
{
    QTC_ASSERT(this != LinuxDeviceConfigurationsPrivate::instance, return);
    Q_ASSERT(idx >= 0 && idx < rowCount());

    const LinuxDeviceConfiguration::ConstPtr &devConf = d->devConfigs.at(idx);
    const LinuxDeviceConfiguration::ConstPtr &oldDefaultDevConf
        = defaultDeviceConfig(devConf->osType());
    if (defaultDeviceConfig(devConf->osType()) == devConf)
        return;
    QModelIndex oldDefaultIndex;
    for (int i = 0; i < d->devConfigs.count(); ++i) {
        if (d->devConfigs.at(i) == oldDefaultDevConf) {
            oldDefaultIndex = index(i, 0);
            break;
        }
    }

    QTC_CHECK(oldDefaultIndex.isValid());
    d->defaultConfigs.insert(devConf->osType(), devConf->internalId());
    emit dataChanged(oldDefaultIndex, oldDefaultIndex);
    const QModelIndex newDefaultIndex = index(idx, 0);
    emit dataChanged(newDefaultIndex, newDefaultIndex);
}

LinuxDeviceConfigurations::LinuxDeviceConfigurations(QObject *parent)
    : QAbstractListModel(parent), d(new LinuxDeviceConfigurationsPrivate)
{
}

LinuxDeviceConfiguration::Ptr LinuxDeviceConfigurations::mutableDeviceAt(int idx) const
{
    Q_ASSERT(idx >= 0 && idx < rowCount());
    return d->devConfigs.at(idx);
}

LinuxDeviceConfigurations::~LinuxDeviceConfigurations()
{
    delete d;
}

void LinuxDeviceConfigurations::load()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    d->nextId = settings->value(IdCounterKey, 1).toULongLong();
    d->defaultSshKeyFilePath = settings->value(DefaultKeyFilePathKey,
        LinuxDeviceConfiguration::defaultPrivateKeyFilePath()).toString();
    const QVariantHash defaultDevsHash = settings->value(QLatin1String(DefaultConfigsKey)).toHash();
    for (QVariantHash::ConstIterator it = defaultDevsHash.constBegin();
            it != defaultDevsHash.constEnd(); ++it) {
        d->defaultConfigs.insert(it.key(), it.value().toULongLong());
    }
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
    const LinuxDeviceConfiguration::Id id = d->defaultConfigs.value(osType,
        LinuxDeviceConfiguration::InvalidId);
    if (id == LinuxDeviceConfiguration::InvalidId)
        return LinuxDeviceConfiguration::ConstPtr();
    return find(id);
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
    foreach (const LinuxDeviceConfiguration::Ptr &devConf, d->devConfigs) {
        if (!defaultDeviceConfig(devConf->osType()))
            d->defaultConfigs.insert(devConf->osType(), devConf->internalId());
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
    if (defaultDeviceConfig(devConf->osType()) == devConf) {
        name += QLatin1Char(' ') + tr("(default for %1)")
            .arg(RemoteLinuxUtils::osTypeToString(devConf->osType()));
    }
    return name;
}

} // namespace RemoteLinux
