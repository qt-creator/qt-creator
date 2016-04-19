/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#ifndef DEBUGGER_DEBUGGERSTARTPARAMETERS_H
#define DEBUGGER_DEBUGGERSTARTPARAMETERS_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <ssh/sshconnection.h>
#include <utils/environment.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/devicesupport/idevice.h>

#include <QMetaType>
#include <QVector>
#include <QPointer>

namespace Debugger {

// Note: This is part of the "soft interface" of the debugger plugin.
// Do not add anything that needs implementation in a .cpp file.

const int InvalidPort = -1;
const qint64 InvalidPid = -1;

class DEBUGGER_EXPORT RemoteSetupResult
{
public:
    int gdbServerPort = InvalidPort;
    int qmlServerPort = InvalidPort;
    qint64 inferiorPid = InvalidPid;
    bool success = false;
    QString reason;
};

class DEBUGGER_EXPORT DebuggerStartParameters
{
public:
    DebuggerStartMode startMode = NoStartMode;
    DebuggerCloseMode closeMode = KillAtClose;

    ProjectExplorer::StandardRunnable inferior;
    QString displayName; // Used in the Snapshots view.
    Utils::Environment debuggerEnvironment;
    Utils::Environment stubEnvironment;
    qint64 attachPID = InvalidPid;
    QStringList solibSearchPath;
    bool useTerminal = false;

    // Used by Qml debugging.
    QString qmlServerAddress;
    int qmlServerPort = InvalidPort;

    // Used by general remote debugging.
    QString remoteChannel;
    QSsh::SshConnectionParameters connParams;
    bool remoteSetupNeeded = false;
    QString symbolFile;

    // Used by Mer plugin (3rd party)
    QMap<QString, QString> sourcePathMap;

    // Used by baremetal plugin
    QByteArray commandsForReset; // commands used for resetting the inferior
    bool useContinueInsteadOfRun = false; // if connected to a hw debugger run is not possible but continue is used
    QByteArray commandsAfterConnect; // additional commands to post after connection to debug target

    // Used by Valgrind
    QVector<QByteArray> expectedSignals;

    // For QNX debugging
    bool useCtrlCStub = false;

    // Used by Android to avoid false positives on warnOnRelease
    bool skipExecutableValidation = false;
    bool useTargetAsync = false;
    QStringList additionalSearchDirectories;

    // Used by iOS.
    QString platform;
    QString deviceSymbolsRoot;
    bool continueAfterAttach = false;
};

} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::RemoteSetupResult)
Q_DECLARE_METATYPE(Debugger::DebuggerStartParameters)

#endif // DEBUGGER_DEBUGGERSTARTPARAMETERS_H

