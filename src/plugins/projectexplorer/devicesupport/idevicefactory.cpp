// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "idevicefactory.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <utils/algorithm.h>
#include <utils/icon.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace ProjectExplorer {

/*!
    \class ProjectExplorer::IDeviceFactory

    \brief The IDeviceFactory class implements an interface for classes that
    provide services related to a certain type of device.

    The factory objects have to be added to the global object pool via
    \c ExtensionSystem::PluginManager::addObject().

    \sa ExtensionSystem::PluginManager::addObject()
*/

/*!
    \fn virtual QString displayName() const = 0

    Returns a short, one-line description of the device type this factory
    can create.
*/

/*!
    \fn virtual IDevice::Ptr create() const
    Creates a new device. This may or may not open a wizard.
*/

/*!
    \fn virtual bool canRestore(const Utils::Storage &map) const = 0

    Checks whether this factory can restore a device from the serialized state
    specified by \a map.
*/

/*!
    \fn virtual IDevice::Ptr restore(const Utils::Storage &map) const = 0

    Loads a device from a serialized state. Only called if \c canRestore()
    returns true for \a map.
*/

/*!
    Checks whether this factory can create new devices. This function is used
    to hide auto-detect-only factories from the listing of possible devices
    to create.
*/

bool IDeviceFactory::canCreate() const
{
    return bool(m_creator);
}

IDevice::Ptr IDeviceFactory::create() const
{
    if (!m_creator)
        return {};

    IDevice::Ptr device = m_creator();
    if (!device) // e.g. Cancel used on the dialog to create a device
        return {};
    return device;
}

IDevice::Ptr IDeviceFactory::construct() const
{
    if (!m_constructor)
        return {};

    IDevice::Ptr device = m_constructor();
    QTC_ASSERT(device, return {});
    device->settings()->displayName.setDefaultValue(displayName());
    return device;
}

static QList<IDeviceFactory *> g_deviceFactories;

IDeviceFactory *IDeviceFactory::find(Utils::Id type)
{
    return Utils::findOrDefault(g_deviceFactories,
        [&type](IDeviceFactory *factory) {
            return factory->deviceType() == type;
        });
}

IDeviceFactory::IDeviceFactory(Utils::Id deviceType)
    : m_deviceType(deviceType)
{
    g_deviceFactories.append(this);
}

void IDeviceFactory::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

void IDeviceFactory::setCombinedIcon(const FilePath &small, const FilePath &large)
{
    using namespace Utils;
    m_icon = Icon::combinedIcon({Icon({{small, Theme::PanelTextColorDark}}, Icon::Tint),
                                 Icon({{large, Theme::IconsBaseColor}})});
}

void IDeviceFactory::setCreator(const std::function<IDevice::Ptr()> &creator)
{
    QTC_ASSERT(creator, return);
    m_creator = creator;
}

void IDeviceFactory::setQuickCreationAllowed(bool on)
{
    m_quickCreationAllowed = on;
}

bool IDeviceFactory::quickCreationAllowed() const
{
    return m_quickCreationAllowed;
}

void IDeviceFactory::setConstructionFunction(const std::function<IDevice::Ptr ()> &constructor)
{
    m_constructor = constructor;
}

void IDeviceFactory::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
}

IDeviceFactory::~IDeviceFactory()
{
    g_deviceFactories.removeOne(this);
}

const QList<IDeviceFactory *> IDeviceFactory::allDeviceFactories()
{
    return g_deviceFactories;
}

} // namespace ProjectExplorer
