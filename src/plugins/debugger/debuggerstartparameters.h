/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
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

#ifndef DEBUGGER_DEBUGGERSTARTPARAMETERS_H
#define DEBUGGER_DEBUGGERSTARTPARAMETERS_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <ssh/sshconnection.h>
#include <utils/environment.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/runconfiguration.h>
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
    RemoteSetupResult()
      : gdbServerPort(InvalidPort),
        qmlServerPort(InvalidPort),
        inferiorPid(InvalidPid),
        success(false)
    {}

    int gdbServerPort;
    int qmlServerPort;
    qint64 inferiorPid;
    bool success;
    QString reason;
};

class DEBUGGER_EXPORT DebuggerStartParameters
{
public:
    DebuggerStartParameters() {}

    DebuggerStartMode startMode = NoStartMode;
    DebuggerCloseMode closeMode = KillAtClose;

    QString executable;
    QString displayName; // Used in the Snapshots view.
    QString processArgs;
    Utils::Environment environment;
    QString workingDirectory;
    qint64 attachPID = InvalidPid;
    QStringList solibSearchPath;
    bool useTerminal = false;

    // Used by Qml debugging.
    QString qmlServerAddress;
    quint16 qmlServerPort;

    // Used by general remote debugging.
    QString remoteChannel;
    QSsh::SshConnectionParameters connParams;
    bool remoteSetupNeeded = false;

    // Used by Mer plugin (3rd party)
    QMap<QString, QString> sourcePathMap;

    // Used by baremetal plugin
    QByteArray commandsForReset; // commands used for resetting the inferior
    bool useContinueInsteadOfRun = false; // if connected to a hw debugger run is not possible but continue is used
    QByteArray commandsAfterConnect; // additional commands to post after connection to debug target

    // Used by Valgrind
    QVector<QByteArray> expectedSignals;

    // For QNX debugging
    QString remoteExecutable;
    bool useCtrlCStub = false;

    // Used by Android to avoid false positives on warnOnRelease
    bool skipExecutableValidation = false;
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

