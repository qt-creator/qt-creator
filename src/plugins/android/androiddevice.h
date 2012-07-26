/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef ANDROIDDEVICE_H
#define ANDROIDDEVICE_H

#include <projectexplorer/devicesupport/idevice.h>

namespace Android {
class AndroidPlugin; // needed for friend declaration

namespace Internal {

class AndroidDevice : public ProjectExplorer::IDevice
{
public:
    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const;

    QString displayType() const;
    ProjectExplorer::IDeviceWidget *createWidget();
    QList<Core::Id> actionIds() const;
    QString displayNameForActionId(Core::Id actionId) const;
    void executeAction(Core::Id actionId, QWidget *parent = 0) const;

    ProjectExplorer::IDevice::Ptr clone() const;

    QString listProcessesCommandLine() const;
    QString killProcessCommandLine(const ProjectExplorer::DeviceProcess &process) const;
    QList<ProjectExplorer::DeviceProcess> buildProcessList(const QString &listProcessesReply) const;

protected:
    friend class AndroidDeviceFactory;
    friend class Android::AndroidPlugin;
    AndroidDevice();
    AndroidDevice(const AndroidDevice &other);
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDDEVICE_H
