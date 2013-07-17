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

#include "idevicefactory.h"

#include <extensionsystem/pluginmanager.h>

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
    \fn virtual QString displayNameForId(Core::Id type) const = 0

    Returns a short, one-line description of the device type.
*/

/*!
    \fn virtual QList<Core::Id> availableCreationIds() const = 0

    Lists the device types this factory can create.
*/

/*!
    \fn virtual IDevice::Ptr create(Core::Id id) const = 0
    Creates a new device with the id \a id. This may or may not open a wizard.
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
    return !availableCreationIds().isEmpty();
}

IDeviceFactory *IDeviceFactory::find(Core::Id type)
{
    QList<IDeviceFactory *> factories
            = ExtensionSystem::PluginManager::getObjects<IDeviceFactory>();
    foreach (IDeviceFactory *factory, factories) {
        if (factory->availableCreationIds().contains(type))
            return factory;
    }
    return 0;
}

IDeviceFactory::IDeviceFactory(QObject *parent) : QObject(parent)
{ }

} // namespace ProjectExplorer
