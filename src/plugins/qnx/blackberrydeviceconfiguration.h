/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QNX_INTERNAL_BLACKBERRYDEVICECONFIGURATION_H
#define QNX_INTERNAL_BLACKBERRYDEVICECONFIGURATION_H

#include <remotelinux/linuxdeviceconfiguration.h>

namespace ProjectExplorer {
class Profile;
}

namespace Qnx {
namespace Internal {

class BlackBerryDeviceConfiguration : public RemoteLinux::LinuxDeviceConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(Qnx::Internal::BlackBerryDeviceConfiguration)
public:
    typedef QSharedPointer<BlackBerryDeviceConfiguration> Ptr;
    typedef QSharedPointer<const BlackBerryDeviceConfiguration> ConstPtr;


    static Ptr create();
    static Ptr create(const QString &name, Core::Id type, MachineType machineType,
                      Origin origin = ManuallyAdded, Core::Id id = Core::Id());

    QString debugToken() const;
    void setDebugToken(const QString &debugToken);

    void fromMap(const QVariantMap &map);

    QString displayType() const;
    ProjectExplorer::IDeviceWidget *createWidget();
    QList<Core::Id> actionIds() const;
    QString displayNameForActionId(Core::Id actionId) const;
    void executeAction(Core::Id actionId, QWidget *parent) const;
    ProjectExplorer::IDevice::Ptr clone() const;

    static ConstPtr device(const ProjectExplorer::Profile *p);

protected:
    BlackBerryDeviceConfiguration();
    BlackBerryDeviceConfiguration(const QString &name, Core::Id type, MachineType machineType,
                             Origin origin, Core::Id id);
    BlackBerryDeviceConfiguration(const BlackBerryDeviceConfiguration &other);

    QVariantMap toMap() const;

private:
    QString m_debugToken;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYDEVICECONFIGURATION_H
