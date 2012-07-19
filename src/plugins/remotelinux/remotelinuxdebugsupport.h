/**************************************************************************
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

#ifndef REMOTELINUXDEBUGSUPPORT_H
#define REMOTELINUXDEBUGSUPPORT_H

#include "remotelinux_export.h"

#include <QObject>

namespace Debugger {
class DebuggerEngine;
class DebuggerStartParameters;
}

namespace ProjectExplorer {
class RunControl;
class RunConfiguration;
}

namespace RemoteLinux {
class RemoteLinuxRunConfiguration;
class AbstractRemoteLinuxApplicationRunner;

namespace Internal {
class AbstractRemoteLinuxDebugSupportPrivate;
class RemoteLinuxDebugSupportPrivate;
} // namespace Internal

class REMOTELINUX_EXPORT AbstractRemoteLinuxDebugSupport : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractRemoteLinuxDebugSupport)
public:
    static Debugger::DebuggerStartParameters startParameters(const RemoteLinuxRunConfiguration *runConfig);

    AbstractRemoteLinuxDebugSupport(ProjectExplorer::RunConfiguration *runConfig,
                                    Debugger::DebuggerEngine *engine);
    ~AbstractRemoteLinuxDebugSupport();

private slots:
    void handleRemoteSetupRequested();
    void handleSshError(const QString &error);
    void startExecution();
    void handleDebuggingFinished();
    void handleRemoteOutput(const QByteArray &output);
    void handleRemoteErrorOutput(const QByteArray &output);
    void handleProgressReport(const QString &progressOutput);
    void handleRemoteProcessStarted();
    void handleRemoteProcessFinished(qint64 exitCode);

private:
    virtual AbstractRemoteLinuxApplicationRunner *runner() const = 0;

    void handleAdapterSetupFailed(const QString &error);
    void handleAdapterSetupDone();
    void setFinished();
    bool setPort(int &port);
    void showMessage(const QString &msg, int channel);

    Internal::AbstractRemoteLinuxDebugSupportPrivate * const d;
};


class REMOTELINUX_EXPORT RemoteLinuxDebugSupport : public AbstractRemoteLinuxDebugSupport
{
    Q_OBJECT
public:
    RemoteLinuxDebugSupport(RemoteLinuxRunConfiguration *runConfig, Debugger::DebuggerEngine *engine);
    ~RemoteLinuxDebugSupport();

private:
    AbstractRemoteLinuxApplicationRunner *runner() const;

    Internal::RemoteLinuxDebugSupportPrivate * const d;
};

} // namespace RemoteLinux

#endif // REMOTELINUXDEBUGSUPPORT_H
