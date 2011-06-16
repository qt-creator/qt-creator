/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef REMOTELINUXDEBUGSUPPORT_H
#define REMOTELINUXDEBUGSUPPORT_H

#include "remotelinux_export.h"
#include "remotelinuxrunconfiguration.h"

#include <debugger/debuggerstartparameters.h>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>

namespace Debugger {
class DebuggerEngine;
}
namespace ProjectExplorer { class RunControl; }

namespace RemoteLinux {
class LinuxDeviceConfiguration;
class RemoteLinuxRunConfiguration;
class RemoteLinuxApplicationRunner;

class REMOTELINUX_EXPORT AbstractRemoteLinuxDebugSupport : public QObject
{
    Q_OBJECT
public:
    static Debugger::DebuggerStartParameters startParameters(const RemoteLinuxRunConfiguration *runConfig);

    AbstractRemoteLinuxDebugSupport(RemoteLinuxRunConfiguration *runConfig, Debugger::DebuggerEngine *engine);
    ~AbstractRemoteLinuxDebugSupport();

private slots:
    void handleAdapterSetupRequested();
    void handleSshError(const QString &error);
    void startExecution();
    void handleDebuggingFinished();
    void handleRemoteOutput(const QByteArray &output);
    void handleRemoteErrorOutput(const QByteArray &output);
    void handleProgressReport(const QString &progressOutput);
    void handleRemoteProcessStarted();
    void handleRemoteProcessFinished(qint64 exitCode);

private:
    enum State { Inactive, StartingRunner, StartingRemoteProcess, Debugging };

    virtual RemoteLinuxApplicationRunner *runner() const=0;

    void handleAdapterSetupFailed(const QString &error);
    void handleAdapterSetupDone();
    void setState(State newState);
    bool setPort(int &port);
    void showMessage(const QString &msg, int channel);

    const QPointer<Debugger::DebuggerEngine> m_engine;
    const QPointer<RemoteLinuxRunConfiguration> m_runConfig;
    const QSharedPointer<const LinuxDeviceConfiguration> m_deviceConfig;
    const RemoteLinuxRunConfiguration::DebuggingType m_debuggingType;

    QByteArray m_gdbserverOutput;
    State m_state;
    int m_gdbServerPort;
    int m_qmlPort;
};


class REMOTELINUX_EXPORT RemoteLinuxDebugSupport : public AbstractRemoteLinuxDebugSupport
{
    Q_OBJECT
public:
    RemoteLinuxDebugSupport(RemoteLinuxRunConfiguration * runConfig, Debugger::DebuggerEngine *engine);
    ~RemoteLinuxDebugSupport();

private:
    RemoteLinuxApplicationRunner *runner() const { return m_runner; }

    RemoteLinuxApplicationRunner * const m_runner;
};

} // namespace RemoteLinux

#endif // REMOTELINUXDEBUGSUPPORT_H
