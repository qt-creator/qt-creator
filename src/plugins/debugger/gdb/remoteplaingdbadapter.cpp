/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
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

#include "remoteplaingdbadapter.h"
#include "gdbengine.h"
#include "debuggerstartparameters.h"

#include <debugger/debuggeractions.h>
#include <debugger/debuggerstringutils.h>
#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

GdbRemotePlainEngine::GdbRemotePlainEngine(const DebuggerStartParameters &startParameters)
    : GdbAbstractPlainEngine(startParameters),
      m_gdbProc(startParameters.connParams, this)
{
    connect(&m_gdbProc, SIGNAL(started()), this, SLOT(handleGdbStarted()));
    connect(&m_gdbProc, SIGNAL(startFailed()), this,
        SLOT(handleGdbStartFailed1()));
}

void GdbRemotePlainEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(QLatin1String("TRYING TO START ADAPTER"));

    if (!startParameters().workingDirectory.isEmpty())
        m_gdbProc.setWorkingDirectory(startParameters().workingDirectory);
    if (startParameters().environment.size())
        m_gdbProc.setEnvironment(startParameters().environment.toStringList());

    notifyEngineRemoteSetupDone(startParameters().connParams.port, startParameters().qmlServerPort);
}

void GdbRemotePlainEngine::setupInferior()
{
    GdbAbstractPlainEngine::setupInferior();
    postCommand("directory " + startParameters().remoteSourcesDir);
}

void GdbRemotePlainEngine::interruptInferior2()
{
    m_gdbProc.interruptInferior();
}

QByteArray GdbRemotePlainEngine::execFilePath() const
{
    return startParameters().executable.toUtf8();
}

QByteArray GdbRemotePlainEngine::toLocalEncoding(const QString &s) const
{
    return s.toUtf8();
}

QString GdbRemotePlainEngine::fromLocalEncoding(const QByteArray &b) const
{
    return QString::fromUtf8(b);
}

void GdbRemotePlainEngine::handleApplicationOutput(const QByteArray &output)
{
    showMessage(QString::fromUtf8(output), AppOutput);
}

void GdbRemotePlainEngine::shutdownEngine()
{
    notifyAdapterShutdownOk();
}

void GdbRemotePlainEngine::notifyEngineRemoteSetupDone(int gdbServerPort, int qmlPort)
{
    Q_UNUSED(gdbServerPort);
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());

    DebuggerStartParameters &sp = startParameters();
    if (qmlPort != -1)
        sp.qmlServerPort = qmlPort;
    m_gdbProc.realStart(sp.debuggerCommand,
        QStringList() << QLatin1String("-i") << QLatin1String("mi"),
        sp.executable);
}

void GdbRemotePlainEngine::handleGdbStarted()
{
     startGdb();
}

void GdbRemotePlainEngine::handleGdbStartFailed1()
{
    handleAdapterStartFailed(m_gdbProc.errorString());
}

void GdbRemotePlainEngine::notifyEngineRemoteSetupFailed(const QString &reason)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    handleAdapterStartFailed(reason);
}

} // namespace Internal
} // namespace Debugger
