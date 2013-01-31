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
