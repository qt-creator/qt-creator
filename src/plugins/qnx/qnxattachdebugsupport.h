/**************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company
** Contact: info@kdab.com
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNX_INTERNAL_QNXATTACHDEBUGSUPPORT_H
#define QNX_INTERNAL_QNXATTACHDEBUGSUPPORT_H

#include <debugger/debuggerconstants.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <projectexplorer/devicesupport/idevice.h>

#include <QObject>

namespace Debugger { class DebuggerRunControl; }

namespace ProjectExplorer {
class DeviceApplicationRunner;
class DeviceUsedPortsGatherer;
class Kit;
}

namespace Qnx {
namespace Internal {

class QnxAttachDebugSupport : public QObject
{
    Q_OBJECT
public:
    explicit QnxAttachDebugSupport(QObject *parent = 0);

public slots:
    void showProcessesDialog();

private slots:
    void launchPDebug();
    void attachToProcess();

    void handleDebuggerStateChanged(Debugger::DebuggerState state);
    void handleError(const QString &message);
    void handleProgressReport(const QString &message);
    void handleRemoteOutput(const QByteArray &output);

private:
    void stopPDebug();

    ProjectExplorer::Kit *m_kit;
    ProjectExplorer::IDevice::ConstPtr m_device;
    ProjectExplorer::DeviceProcessItem m_process;

    ProjectExplorer::DeviceApplicationRunner *m_runner;
    ProjectExplorer::DeviceUsedPortsGatherer *m_portsGatherer;
    Debugger::DebuggerRunControl *m_runControl;

    int m_pdebugPort;
    QString m_projectSourceDirectory;
    QString m_localExecutablePath;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_QNXATTACHDEBUGSUPPORT_H
