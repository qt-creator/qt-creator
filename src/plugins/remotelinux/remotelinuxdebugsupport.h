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

#ifndef REMOTELINUXDEBUGSUPPORT_H
#define REMOTELINUXDEBUGSUPPORT_H

#include "remotelinux_export.h"

#include <QObject>

namespace Debugger {
class DebuggerEngine;
class DebuggerStartParameters;
}

namespace ProjectExplorer {
class DeviceApplicationHelperAction;
class RunConfiguration;
}

namespace RemoteLinux {
class RemoteLinuxRunConfiguration;

namespace Internal { class LinuxDeviceDebugSupportPrivate; }

class REMOTELINUX_EXPORT LinuxDeviceDebugSupport : public QObject
{
    Q_OBJECT
public:
    static Debugger::DebuggerStartParameters startParameters(const RemoteLinuxRunConfiguration *runConfig);

    LinuxDeviceDebugSupport(ProjectExplorer::RunConfiguration *runConfig,
            Debugger::DebuggerEngine *engine);
    ~LinuxDeviceDebugSupport();

    void setApplicationRunnerPreRunAction(ProjectExplorer::DeviceApplicationHelperAction *action);
    void setApplicationRunnerPostRunAction(ProjectExplorer::DeviceApplicationHelperAction *action);

private slots:
    void handleRemoteSetupRequested();
    void handleAppRunnerError(const QString &error);
    void startExecution();
    void handleDebuggingFinished();
    void handleRemoteOutput(const QByteArray &output);
    void handleRemoteErrorOutput(const QByteArray &output);
    void handleProgressReport(const QString &progressOutput);
    void handleRemoteProcessStarted();
    void handleAppRunnerFinished(bool success);
    void handlePortsGathererError(const QString &message);
    void handlePortListReady();

private:
    void handleAdapterSetupFailed(const QString &error);
    void handleAdapterSetupDone();
    void setFinished();
    bool setPort(int &port);
    void showMessage(const QString &msg, int channel);

    Internal::LinuxDeviceDebugSupportPrivate * const d;
};

} // namespace RemoteLinux

#endif // REMOTELINUXDEBUGSUPPORT_H
