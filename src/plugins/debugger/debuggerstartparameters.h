/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_DEBUGGERSTARTPARAMETERS_H
#define DEBUGGER_DEBUGGERSTARTPARAMETERS_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <coreplugin/ssh/sshconnection.h>
#include <utils/environment.h>
#include <projectexplorer/toolchaintype.h>

#include <QtCore/QMetaType>

namespace Debugger {

// Note: This is part of the "soft interface" of the debugger plugin.
// Do not add anything that needs implementation in a .cpp file.

class DEBUGGER_EXPORT DebuggerStartParameters
{
public:
    DebuggerStartParameters()
      : isSnapshot(false),
        attachPID(-1),
        useTerminal(false),
        enabledEngines(AllEngineTypes),
        qmlServerAddress(QLatin1String("127.0.0.1")),
        qmlServerPort(0),
        useServerStartScript(false),
        connParams(Core::SshConnectionParameters::NoProxy),
        toolChainType(ProjectExplorer::ToolChain_UNKNOWN),
        startMode(NoStartMode),
        executableUid(0)
    {}

    QString executable;
    QString displayName;
    QString coreFile;
    bool isSnapshot; // Set if created internally.
    QString processArgs;
    Utils::Environment environment;
    QString workingDirectory;
    qint64 attachPID;
    bool useTerminal;
    unsigned enabledEngines;

    // Used by AttachCrashedExternal.
    QString crashParameter;

    // Used by Qml debugging.
    QString qmlServerAddress;
    quint16 qmlServerPort;
    QString projectBuildDir;
    QString projectDir;

    // Used by combined cpp+qml debugging.
    DebuggerEngineType cppEngineType;

    // Used by remote debugging.
    QString remoteChannel;
    QString remoteArchitecture;
    QString gnuTarget;
    QString symbolFileName;
    bool useServerStartScript;
    QString serverStartScript;
    QString sysRoot;
    QByteArray remoteDumperLib;
    QByteArray remoteSourcesDir;
    QString remoteMountPoint;
    QString localMountDir;
    Core::SshConnectionParameters connParams;

    QString debuggerCommand;
    ProjectExplorer::ToolChainType toolChainType;
    QString qtInstallPath;

    QString dumperLibrary;
    QStringList dumperLibraryLocations;
    DebuggerStartMode startMode;

    // For Symbian debugging.
    quint32 executableUid;
};

} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::DebuggerStartParameters)

#endif // DEBUGGER_DEBUGGERSTARTPARAMETERS_H

