/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "kitinformation.h"

#include "devicesupport/desktopdevice.h"
#include "devicesupport/devicemanager.h"
#include "projectexplorerconstants.h"
#include "kit.h"
#include "kitinformationconfigwidget.h"
#include "toolchain.h"
#include "toolchainmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/abi.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// SysRootInformation:
// --------------------------------------------------------------------------

static const char SYSROOT_INFORMATION[] = "PE.Profile.SysRoot";

SysRootKitInformation::SysRootKitInformation()
{
    setObjectName(QLatin1String("SysRootInformation"));
}

Core::Id SysRootKitInformation::dataId() const
{
    static const Core::Id id(SYSROOT_INFORMATION);
    return id;
}

unsigned int SysRootKitInformation::priority() const
{
    return 31000;
}

QVariant SysRootKitInformation::defaultValue(Kit *k) const
{
    Q_UNUSED(k)
    return QString();
}

QList<Task> SysRootKitInformation::validate(const Kit *k) const
{
    QList<Task> result;
    const Utils::FileName dir = SysRootKitInformation::sysRoot(k);
    if (!dir.toFileInfo().isDir() && SysRootKitInformation::hasSysRoot(k)) {
        result << Task(Task::Error, tr("Sys Root \"%1\" is not a directory.").arg(dir.toUserOutput()),
                       Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    return result;
}

KitConfigWidget *SysRootKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::SysRootInformationConfigWidget(k, isSticky(k));
}

KitInformation::ItemList SysRootKitInformation::toUserOutput(const Kit *k) const
{
    return ItemList() << qMakePair(tr("Sys Root"), sysRoot(k).toUserOutput());
}

bool SysRootKitInformation::hasSysRoot(const Kit *k)
{
    if (k)
        return !k->value(Core::Id(SYSROOT_INFORMATION)).toString().isEmpty();
    return false;
}

Utils::FileName SysRootKitInformation::sysRoot(const Kit *k)
{
    if (!k)
        return Utils::FileName();
    return Utils::FileName::fromString(k->value(Core::Id(SYSROOT_INFORMATION)).toString());
}

void SysRootKitInformation::setSysRoot(Kit *k, const Utils::FileName &v)
{
    k->setValue(Core::Id(SYSROOT_INFORMATION), v.toString());
}

// --------------------------------------------------------------------------
// ToolChainInformation:
// --------------------------------------------------------------------------

static const char TOOLCHAIN_INFORMATION[] = "PE.Profile.ToolChain";

ToolChainKitInformation::ToolChainKitInformation()
{
    setObjectName(QLatin1String("ToolChainInformation"));
    connect(KitManager::instance(), SIGNAL(kitsLoaded()),
            this, SLOT(kitsWereLoaded()));
}

Core::Id ToolChainKitInformation::dataId() const
{
    static const Core::Id id(TOOLCHAIN_INFORMATION);
    return id;
}

unsigned int ToolChainKitInformation::priority() const
{
    return 30000;
}

QVariant ToolChainKitInformation::defaultValue(Kit *k) const
{
    Q_UNUSED(k);
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

QList<Task> ToolChainKitInformation::validate(const Kit *k) const
{
    QList<Task> result;

    const ToolChain* toolchain = toolChain(k);
    if (!toolchain) {
        result << Task(Task::Error, ToolChainKitInformation::msgNoToolChainInTarget(),
                       Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
    } else {
        result << toolchain->validateKit(k);
    }
    return result;
}

void ToolChainKitInformation::fix(Kit *k)
{
    QTC_ASSERT(ToolChainManager::instance()->isLoaded(), return);
    if (toolChain(k))
        return;

    qWarning("No tool chain set from kit \"%s\".",
             qPrintable(k->displayName()));
    setToolChain(k, 0); // make sure to clear out no longer known tool chains
}

void ToolChainKitInformation::setup(Kit *k)
{
    QTC_ASSERT(ToolChainManager::instance()->isLoaded(), return);
    const QString id = k->value(Core::Id(TOOLCHAIN_INFORMATION)).toString();
    if (id.isEmpty())
        return;

    ToolChain *tc = ToolChainManager::instance()->findToolChain(id);
    if (tc)
        return;

    // ID is not found: Might be an ABI string...
    foreach (ToolChain *current, ToolChainManager::instance()->toolChains()) {
        if (current->targetAbi().toString() == id)
            return setToolChain(k, current);
    }
}

KitConfigWidget *ToolChainKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::ToolChainInformationConfigWidget(k, isSticky(k));
}

QString ToolChainKitInformation::displayNamePostfix(const Kit *k) const
{
    ToolChain *tc = toolChain(k);
    return tc ? tc->displayName() : QString();
}

KitInformation::ItemList ToolChainKitInformation::toUserOutput(const Kit *k) const
{
    ToolChain *tc = toolChain(k);
    return ItemList() << qMakePair(tr("Compiler"), tc ? tc->displayName() : tr("None"));
}

void ToolChainKitInformation::addToEnvironment(const Kit *k, Utils::Environment &env) const
{
    ToolChain *tc = toolChain(k);
    if (tc)
        tc->addToEnvironment(env);
}

IOutputParser *ToolChainKitInformation::createOutputParser(const Kit *k) const
{
    ToolChain *tc = toolChain(k);
    if (tc)
        return tc->outputParser();
    return 0;
}

ToolChain *ToolChainKitInformation::toolChain(const Kit *k)
{
    QTC_ASSERT(ToolChainManager::instance()->isLoaded(), return 0);
    if (!k)
        return 0;
    return ToolChainManager::instance()
            ->findToolChain(k->value(Core::Id(TOOLCHAIN_INFORMATION)).toString());
}

void ToolChainKitInformation::setToolChain(Kit *k, ToolChain *tc)
{
    k->setValue(Core::Id(TOOLCHAIN_INFORMATION), tc ? tc->id() : QString());
}

QString ToolChainKitInformation::msgNoToolChainInTarget()
{
    return tr("No compiler set in kit.");
}

void ToolChainKitInformation::kitsWereLoaded()
{
    foreach (Kit *k, KitManager::instance()->kits())
        fix(k);

    connect(ToolChainManager::instance(), SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
            this, SLOT(toolChainRemoved(ProjectExplorer::ToolChain*)));
    connect(ToolChainManager::instance(), SIGNAL(toolChainUpdated(ProjectExplorer::ToolChain*)),
            this, SLOT(toolChainUpdated(ProjectExplorer::ToolChain*)));
}

void ToolChainKitInformation::toolChainUpdated(ProjectExplorer::ToolChain *tc)
{
    ToolChainMatcher m(tc);
    foreach (Kit *k, KitManager::instance()->kits(&m))
        notifyAboutUpdate(k);
}

void ToolChainKitInformation::toolChainRemoved(ProjectExplorer::ToolChain *tc)
{
    Q_UNUSED(tc);
    foreach (Kit *k, KitManager::instance()->kits())
        fix(k);
}

// --------------------------------------------------------------------------
// DeviceTypeInformation:
// --------------------------------------------------------------------------

static const char DEVICETYPE_INFORMATION[] = "PE.Profile.DeviceType";

DeviceTypeKitInformation::DeviceTypeKitInformation()
{
    setObjectName(QLatin1String("DeviceTypeInformation"));
}

Core::Id DeviceTypeKitInformation::dataId() const
{
    static const Core::Id id(DEVICETYPE_INFORMATION);
    return id;
}

unsigned int DeviceTypeKitInformation::priority() const
{
    return 33000;
}

QVariant DeviceTypeKitInformation::defaultValue(Kit *k) const
{
    Q_UNUSED(k);
    return QByteArray(Constants::DESKTOP_DEVICE_TYPE);
}

QList<Task> DeviceTypeKitInformation::validate(const Kit *k) const
{
    Q_UNUSED(k);
    return QList<Task>();
}

KitConfigWidget *DeviceTypeKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::DeviceTypeInformationConfigWidget(k, isSticky(k));
}

KitInformation::ItemList DeviceTypeKitInformation::toUserOutput(const Kit *k) const
{
    Core::Id type = deviceTypeId(k);
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

const Core::Id DeviceTypeKitInformation::deviceTypeId(const Kit *k)
{
    return k ? Core::Id::fromSetting(k->value(DEVICETYPE_INFORMATION)) : Core::Id();
}

void DeviceTypeKitInformation::setDeviceTypeId(Kit *k, Core::Id type)
{
    k->setValue(DEVICETYPE_INFORMATION, type.toSetting());
}

// --------------------------------------------------------------------------
// DeviceInformation:
// --------------------------------------------------------------------------

static const char DEVICE_INFORMATION[] = "PE.Profile.Device";

DeviceKitInformation::DeviceKitInformation()
{
    setObjectName(QLatin1String("DeviceInformation"));
    connect(KitManager::instance(), SIGNAL(kitsLoaded()),
            this, SLOT(kitsWereLoaded()));
}

Core::Id DeviceKitInformation::dataId() const
{
    static const Core::Id id(DEVICE_INFORMATION);
    return id;
}

unsigned int DeviceKitInformation::priority() const
{
    return 32000;
}

QVariant DeviceKitInformation::defaultValue(Kit *k) const
{
    Core::Id type = DeviceTypeKitInformation::deviceTypeId(k);
    IDevice::ConstPtr dev = DeviceManager::instance()->defaultDevice(type);
    return dev.isNull() ? QString() : dev->id().toString();
}

QList<Task> DeviceKitInformation::validate(const Kit *k) const
{
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    QList<Task> result;
    if (!dev.isNull() && dev->type() != DeviceTypeKitInformation::deviceTypeId(k))
        result.append(Task(Task::Error, tr("Device does not match device type."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)));
    if (dev.isNull())
        result.append(Task(Task::Warning, tr("No device set."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)));
    return result;
}

void DeviceKitInformation::fix(Kit *k)
{
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    if (!dev.isNull() && dev->type() == DeviceTypeKitInformation::deviceTypeId(k))
        return;

    qWarning("Device is no longer known, removing from kit \"%s\".", qPrintable(k->displayName()));
    setDeviceId(k, Core::Id());
}

void DeviceKitInformation::setup(Kit *k)
{
    QTC_ASSERT(DeviceManager::instance()->isLoaded(), return);
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    if (!dev.isNull() && dev->type() == DeviceTypeKitInformation::deviceTypeId(k))
        return;

    setDeviceId(k, Core::Id::fromSetting(defaultValue(k)));
}

KitConfigWidget *DeviceKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::DeviceInformationConfigWidget(k, isSticky(k));
}

QString DeviceKitInformation::displayNamePostfix(const Kit *k) const
{
    IDevice::ConstPtr dev = device(k);
    return dev.isNull() ? QString() : dev->displayName();
}

KitInformation::ItemList DeviceKitInformation::toUserOutput(const Kit *k) const
{
    IDevice::ConstPtr dev = device(k);
    return ItemList() << qMakePair(tr("Device"), dev.isNull() ? tr("Unconfigured") : dev->displayName());
}

IDevice::ConstPtr DeviceKitInformation::device(const Kit *k)
{
    QTC_ASSERT(DeviceManager::instance()->isLoaded(), return IDevice::ConstPtr());
    return DeviceManager::instance()->find(deviceId(k));
}

Core::Id DeviceKitInformation::deviceId(const Kit *k)
{
    return k ? Core::Id::fromSetting(k->value(DEVICE_INFORMATION)) : Core::Id();
}

void DeviceKitInformation::setDevice(Kit *k, IDevice::ConstPtr dev)
{
    setDeviceId(k, dev ? dev->id() : Core::Id());
}

void DeviceKitInformation::setDeviceId(Kit *k, const Core::Id id)
{
    k->setValue(DEVICE_INFORMATION, id.toSetting());
}

void DeviceKitInformation::kitsWereLoaded()
{
    foreach (Kit *k, KitManager::instance()->kits())
        fix(k);

    connect(DeviceManager::instance(), SIGNAL(deviceAdded(Core::Id)),
            this, SLOT(deviceAdded(Core::Id)));
    connect(DeviceManager::instance(), SIGNAL(deviceRemoved(Core::Id)),
            this, SLOT(deviceRemoved(Core::Id)));
    connect(DeviceManager::instance(), SIGNAL(deviceUpdated(Core::Id)),
            this, SLOT(deviceUpdated(Core::Id)));

    connect(KitManager::instance(), SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(kitUpdated(ProjectExplorer::Kit*)));
    connect(KitManager::instance(), SIGNAL(unmanagedKitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(kitUpdated(ProjectExplorer::Kit*)));
}

void DeviceKitInformation::deviceUpdated(const Core::Id &id)
{
    foreach (Kit *k, KitManager::instance()->kits()) {
        if (deviceId(k) == id)
            notifyAboutUpdate(k);
    }
}

void DeviceKitInformation::kitUpdated(Kit *k)
{
    setup(k); // Set default device if necessary
}

void DeviceKitInformation::deviceAdded(const Core::Id &id)
{
    Q_UNUSED(id);
    DeviceMatcher m;
    foreach (Kit *k, KitManager::instance()->kits(&m)) {
        setup(k); // Set default device if necessary
    }
}

void DeviceKitInformation::deviceRemoved(const Core::Id &id)
{
    DeviceMatcher m(id);
    foreach (Kit *k, KitManager::instance()->kits(&m))
        setup(k); // Set default device if necessary
}

} // namespace ProjectExplorer
