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

#include "profileinformation.h"

#include "devicesupport/desktopdevice.h"
#include "devicesupport/devicemanager.h"
#include "projectexplorerconstants.h"
#include "profile.h"
#include "profileinformationconfigwidget.h"
#include "toolchain.h"
#include "toolchainmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/abi.h>
#include <utils/pathchooser.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// SysRootInformation:
// --------------------------------------------------------------------------

static const char SYSROOT_INFORMATION[] = "PE.Profile.SysRoot";

SysRootProfileInformation::SysRootProfileInformation()
{
    setObjectName(QLatin1String("SysRootInformation"));
}

Core::Id SysRootProfileInformation::dataId() const
{
    static const Core::Id id(SYSROOT_INFORMATION);
    return id;
}

unsigned int SysRootProfileInformation::priority() const
{
    return 32000;
}

QVariant SysRootProfileInformation::defaultValue(Profile *p) const
{
    Q_UNUSED(p)
    return QString();
}

QList<Task> SysRootProfileInformation::validate(Profile *p) const
{
    QList<Task> result;
    const Utils::FileName dir = SysRootProfileInformation::sysRoot(p);
    if (!dir.toFileInfo().isDir() && SysRootProfileInformation::hasSysRoot(p))
        result << Task(Task::Error, QObject::tr("Sys Root \"%1\" is not a directory.").arg(dir.toUserOutput()),
                       Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
    return result;
}

ProfileConfigWidget *SysRootProfileInformation::createConfigWidget(Profile *p) const
{
    Q_ASSERT(p);
    return new Internal::SysRootInformationConfigWidget(p);
}

ProfileInformation::ItemList SysRootProfileInformation::toUserOutput(Profile *p) const
{
    return ItemList() << qMakePair(tr("Sys Root"), sysRoot(p).toUserOutput());
}

bool SysRootProfileInformation::hasSysRoot(const Profile *p)
{
    return !p->value(Core::Id(SYSROOT_INFORMATION)).toString().isEmpty();
}

Utils::FileName SysRootProfileInformation::sysRoot(const Profile *p)
{
    if (!p)
        return Utils::FileName();
    return Utils::FileName::fromString(p->value(Core::Id(SYSROOT_INFORMATION)).toString());
}

void SysRootProfileInformation::setSysRoot(Profile *p, const Utils::FileName &v)
{
    p->setValue(Core::Id(SYSROOT_INFORMATION), v.toString());
}

// --------------------------------------------------------------------------
// ToolChainInformation:
// --------------------------------------------------------------------------

static const char TOOLCHAIN_INFORMATION[] = "PE.Profile.ToolChain";

ToolChainProfileInformation::ToolChainProfileInformation()
{
    setObjectName(QLatin1String("ToolChainInformation"));
    connect(ToolChainManager::instance(), SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
            this, SIGNAL(validationNeeded()));
    connect(ToolChainManager::instance(), SIGNAL(toolChainUpdated(ProjectExplorer::ToolChain*)),
            this, SIGNAL(validationNeeded()));
}

Core::Id ToolChainProfileInformation::dataId() const
{
    static const Core::Id id(TOOLCHAIN_INFORMATION);
    return id;
}

unsigned int ToolChainProfileInformation::priority() const
{
    return 30000;
}

QVariant ToolChainProfileInformation::defaultValue(Profile *p) const
{
    Q_UNUSED(p);
    QList<ToolChain *> tcList = ToolChainManager::instance()->toolChains();
    if (tcList.isEmpty())
        return QString();

    ProjectExplorer::Abi abi = ProjectExplorer::Abi::hostAbi();

    foreach (ToolChain *tc, tcList) {
        if (tc->targetAbi() == abi)
            return tc->id();
    }

    return tcList.at(0)->id();
}

QList<Task> ToolChainProfileInformation::validate(Profile *p) const
{
    QList<Task> result;
    if (!toolChain(p)) {
        setToolChain(p, 0); // make sure to clear out no longer known tool chains
        result << Task(Task::Error, QObject::tr("No tool chain set up."),
                       Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    return result;
}

ProfileConfigWidget *ToolChainProfileInformation::createConfigWidget(Profile *p) const
{
    Q_ASSERT(p);
    return new Internal::ToolChainInformationConfigWidget(p);
}

ProfileInformation::ItemList ToolChainProfileInformation::toUserOutput(Profile *p) const
{
    ToolChain *tc = toolChain(p);
    return ItemList() << qMakePair(tr("Tool chain"), tc ? tc->displayName() : tr("None"));
}

void ToolChainProfileInformation::addToEnvironment(const Profile *p, Utils::Environment &env) const
{
    ToolChain *tc = toolChain(p);
    if (tc)
        tc->addToEnvironment(env);
}

ToolChain *ToolChainProfileInformation::toolChain(const Profile *p)
{
    if (!p)
        return 0;
    const QString id = p->value(Core::Id(TOOLCHAIN_INFORMATION)).toString();
    return ToolChainManager::instance()->findToolChain(id);
}

void ToolChainProfileInformation::setToolChain(Profile *p, ToolChain *tc)
{
    p->setValue(Core::Id(TOOLCHAIN_INFORMATION), tc ? tc->id() : QString());
}

// --------------------------------------------------------------------------
// DeviceTypeInformation:
// --------------------------------------------------------------------------

static const char DEVICETYPE_INFORMATION[] = "PE.Profile.DeviceType";

DeviceTypeProfileInformation::DeviceTypeProfileInformation()
{
    setObjectName(QLatin1String("DeviceTypeInformation"));
}

Core::Id DeviceTypeProfileInformation::dataId() const
{
    static const Core::Id id(DEVICETYPE_INFORMATION);
    return id;
}

unsigned int DeviceTypeProfileInformation::priority() const
{
    return 33000;
}

QVariant DeviceTypeProfileInformation::defaultValue(Profile *p) const
{
    Q_UNUSED(p);
    return QByteArray(Constants::DESKTOP_DEVICE_TYPE);
}

QList<Task> DeviceTypeProfileInformation::validate(Profile *p) const
{
    IDevice::ConstPtr dev = DeviceProfileInformation::device(p);
    QList<Task> result;
    if (!dev.isNull() && dev->type() != DeviceTypeProfileInformation::deviceTypeId(p))
        result.append(Task(Task::Error, QObject::tr("Device does not match device type."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)));
    return result;
}

ProfileConfigWidget *DeviceTypeProfileInformation::createConfigWidget(Profile *p) const
{
    Q_ASSERT(p);
    return new Internal::DeviceTypeInformationConfigWidget(p);
}

ProfileInformation::ItemList DeviceTypeProfileInformation::toUserOutput(Profile *p) const
{
    Core::Id type = deviceTypeId(p);
    QString typeDisplayName = tr("Unknown device type");
    if (type.isValid()) {
        QList<IDeviceFactory *> factories
                = ExtensionSystem::PluginManager::instance()->getObjects<IDeviceFactory>();
        foreach (IDeviceFactory *factory, factories) {
            if (factory->availableCreationIds().contains(type)) {
                typeDisplayName = factory->displayNameForId(type);
                break;
            }
        }
    }
    return ItemList() << qMakePair(tr("Device type"), typeDisplayName);
}

const Core::Id DeviceTypeProfileInformation::deviceTypeId(const Profile *p)
{
    if (!p)
        return Core::Id();
    return Core::Id(p->value(Core::Id(DEVICETYPE_INFORMATION)).toByteArray().constData());
}

void DeviceTypeProfileInformation::setDeviceTypeId(Profile *p, Core::Id type)
{
    p->setValue(Core::Id(DEVICETYPE_INFORMATION), type.name());
}

// --------------------------------------------------------------------------
// DeviceInformation:
// --------------------------------------------------------------------------

static const char DEVICE_INFORMATION[] = "PE.Profile.Device";

DeviceProfileInformation::DeviceProfileInformation()
{
    setObjectName(QLatin1String("DeviceInformation"));
    connect(DeviceManager::instance(), SIGNAL(deviceRemoved(Core::Id)),
            this, SIGNAL(validationNeeded()));
    connect(DeviceManager::instance(), SIGNAL(deviceUpdated(Core::Id)),
            this, SIGNAL(validationNeeded()));
}

Core::Id DeviceProfileInformation::dataId() const
{
    static const Core::Id id(DEVICE_INFORMATION);
    return id;
}

unsigned int DeviceProfileInformation::priority() const
{
    return 32000;
}

QVariant DeviceProfileInformation::defaultValue(Profile *p) const
{
    Q_UNUSED(p);
    return QByteArray(Constants::DESKTOP_DEVICE_ID);
}

QList<Task> DeviceProfileInformation::validate(Profile *p) const
{
    Q_UNUSED(p);
    QList<Task> result;
    return result;
}

ProfileConfigWidget *DeviceProfileInformation::createConfigWidget(Profile *p) const
{
    Q_ASSERT(p);
    return new Internal::DeviceInformationConfigWidget(p);
}

ProfileInformation::ItemList DeviceProfileInformation::toUserOutput(Profile *p) const
{
    IDevice::ConstPtr dev = device(p);
    return ItemList() << qMakePair(tr("Device"), dev.isNull() ? tr("Unconfigured") : dev->displayName());
}

IDevice::ConstPtr DeviceProfileInformation::device(const Profile *p)
{
    DeviceManager *dm = DeviceManager::instance();
    return dm ? dm->find(deviceId(p)) : IDevice::ConstPtr();
}

Core::Id DeviceProfileInformation::deviceId(const Profile *p)
{
    if (p) {
        QByteArray idname = p->value(Core::Id(DEVICE_INFORMATION)).toByteArray();
        return idname.isEmpty() ? IDevice::invalidId() : Core::Id(idname.constData());
    }
    return IDevice::invalidId();
}

void DeviceProfileInformation::setDevice(Profile *p, IDevice::ConstPtr dev)
{
    setDeviceId(p, dev ? dev->id() : IDevice::invalidId());
}

void DeviceProfileInformation::setDeviceId(Profile *p, const Core::Id id)
{
    p->setValue(Core::Id(DEVICE_INFORMATION), id.name());
}

} // namespace ProjectExplorer
