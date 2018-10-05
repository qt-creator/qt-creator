/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "idevicefactory.h"

#include <utils/algorithm.h>
#include <utils/icon.h>

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
    \fn virtual bool canRestore(const QVariantMap &map) const = 0

    Checks whether this factory can restore a device from the serialized state
    specified by \a map.
*/

/*!
    \fn virtual IDevice::Ptr restore(const QVariantMap &map) const = 0

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
    return m_canCreate;
}

IDevice::Ptr IDeviceFactory::construct() const
{
    return m_constructor ? m_constructor() : IDevice::Ptr();
}

static QList<IDeviceFactory *> g_deviceFactories;

IDeviceFactory *IDeviceFactory::find(Core::Id type)
{
    return Utils::findOrDefault(g_deviceFactories,
        [&type](IDeviceFactory *factory) {
            return factory->deviceType() == type;
        });
}

IDeviceFactory::IDeviceFactory(Core::Id deviceType)
    : m_deviceType(deviceType)
{
    g_deviceFactories.append(this);
}

void IDeviceFactory::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

void IDeviceFactory::setCombinedIcon(const QString &small, const QString &large)
{
    using namespace Utils;
    m_icon = Icon::combinedIcon({Icon({{small, Theme::PanelTextColorDark}}, Icon::Tint),
                                 Icon({{large, Theme::IconsBaseColor}})});
}

void IDeviceFactory::setCanCreate(bool canCreate)
{
    m_canCreate = canCreate;
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
