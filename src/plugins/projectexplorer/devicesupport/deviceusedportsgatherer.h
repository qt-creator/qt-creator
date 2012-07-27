/*************DeviceUsedPortsGatherer*********************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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

#ifndef DEVICEUSEDPORTSGATHERER_H
#define DEVICEUSEDPORTSGATHERER_H

#include "idevice.h"

namespace Utils { class PortList; }

namespace ProjectExplorer {
namespace Internal { class DeviceUsedPortsGathererPrivate; }

class PROJECTEXPLORER_EXPORT DeviceUsedPortsGatherer : public QObject
{
    Q_OBJECT

public:
    DeviceUsedPortsGatherer(QObject *parent = 0);
    ~DeviceUsedPortsGatherer();

    void start(const ProjectExplorer::IDevice::ConstPtr &device);
    void stop();
    int getNextFreePort(Utils::PortList *freePorts) const; // returns -1 if no more are left
    QList<int> usedPorts() const;

    void setCommand(const QString &command); // Will use default command if not set

signals:
    void error(const QString &errMsg);
    void portListReady();

private slots:
    void handleConnectionEstablished();
    void handleConnectionError();
    void handleProcessClosed(int exitStatus);
    void handleRemoteStdOut();
    void handleRemoteStdErr();

private:
    void setupUsedPorts();

    Internal::DeviceUsedPortsGathererPrivate * const d;
};

} // namespace ProjectExplorer

#endif // DEVICEUSEDPORTSGATHERER_H
