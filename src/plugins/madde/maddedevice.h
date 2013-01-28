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

#ifndef MADDEDEVICE_H
#define MADDEDEVICE_H

#include <remotelinux/linuxdevice.h>

#include <QCoreApplication>

namespace Madde {
namespace Internal {

class MaddeDevice : public RemoteLinux::LinuxDevice
{
    Q_DECLARE_TR_FUNCTIONS(Madde::Internal::MaddeDevice)
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
