/****************************************************************************
**
** Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company
** Contact: info@kdab.com
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

#pragma once

#include <debugger/debuggerconstants.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <utils/port.h>

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

    ProjectExplorer::Kit *m_kit = 0;
    ProjectExplorer::IDevice::ConstPtr m_device;
    ProjectExplorer::DeviceProcessItem m_process;

    ProjectExplorer::DeviceApplicationRunner *m_runner;
    ProjectExplorer::DeviceUsedPortsGatherer *m_portsGatherer;
    Debugger::DebuggerRunControl *m_runControl = 0;

    Utils::Port m_pdebugPort;
    QString m_projectSourceDirectory;
    QString m_localExecutablePath;
};

} // namespace Internal
} // namespace Qnx
