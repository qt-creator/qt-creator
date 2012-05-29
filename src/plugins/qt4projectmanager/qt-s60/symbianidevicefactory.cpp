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

#include "symbianidevicefactory.h"

#include "symbianidevice.h"

#include <utils/qtcassert.h>

namespace Qt4ProjectManager {
namespace Internal {

SymbianIDeviceFactory::SymbianIDeviceFactory(QObject *parent) : IDeviceFactory(parent)
{ }

QString SymbianIDeviceFactory::displayNameForId(Core::Id type) const
{
    if (type == deviceType())
        return tr("Symbian Device");
    return QString();
}

QList<Core::Id> SymbianIDeviceFactory::availableCreationIds() const
{
    return QList<Core::Id>() << deviceType();
}

bool SymbianIDeviceFactory::canCreate() const
{
    return false;
}

ProjectExplorer::IDevice::Ptr SymbianIDeviceFactory::create(Core::Id id) const
{
    Q_UNUSED(id);
    return ProjectExplorer::IDevice::Ptr();
}

bool SymbianIDeviceFactory::canRestore(const QVariantMap &map) const
{
    return ProjectExplorer::IDevice::typeFromMap(map) == deviceType();
}

ProjectExplorer::IDevice::Ptr SymbianIDeviceFactory::restore(const QVariantMap &map) const
{
    QTC_ASSERT(canRestore(map), return ProjectExplorer::IDevice::Ptr());
    SymbianIDevice *dev = new SymbianIDevice(map);
    return ProjectExplorer::IDevice::Ptr(dev);
}

Core::Id SymbianIDeviceFactory::deviceType()
{
    return Core::Id("Qt4ProjectManager.SymbianDevice");
}

} // namespace internal
} // namespace qt4projectmanager
