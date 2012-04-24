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
#ifndef MADDEDEVICE_H
#define MADDEDEVICE_H

#include <remotelinux/linuxdeviceconfiguration.h>

#include <QCoreApplication>

namespace Madde {
namespace Internal {

class MaddeDevice : public RemoteLinux::LinuxDeviceConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(MaddeDevice)
public:
    typedef QSharedPointer<MaddeDevice> Ptr;
    typedef QSharedPointer<const MaddeDevice> ConstPtr;

    static Ptr create();
    static Ptr create(const QString &name, Core::Id type, MachineType machineType,
                      Origin origin = ManuallyAdded, Core::Id id = Core::Id());

    QString displayType() const;
    QList<Core::Id> actionIds() const;
    QString displayNameForActionId(Core::Id actionId) const;
    void executeAction(Core::Id actionId, QWidget *parent) const;
    ProjectExplorer::IDevice::Ptr clone() const;
    static QString maddeDisplayType(Core::Id type);

    static bool allowsRemoteMounts(Core::Id type);
    static bool allowsPackagingDisabling(Core::Id type);
    static bool allowsQmlDebugging(Core::Id type);

    static bool isDebianBased(Core::Id type);
    static QSize packageManagerIconSize(Core::Id type);

private:
    MaddeDevice();
    MaddeDevice(const QString &name, Core::Id type, MachineType machineType,
                Origin origin, Core::Id id);

    MaddeDevice(const MaddeDevice &other);
};

} // namespace Internal
} // namespace Madde

#endif // MADDEDEVICE_H
