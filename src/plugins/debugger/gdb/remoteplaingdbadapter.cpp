/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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

RemotePlainGdbAdapter::RemotePlainGdbAdapter(GdbEngine *engine)
    : AbstractPlainGdbAdapter(engine),
      m_gdbProc(engine->startParameters().connParams, this)
{
    connect(&m_gdbProc, SIGNAL(started()), this, SLOT(handleGdbStarted()));
    connect(&m_gdbProc, SIGNAL(startFailed()), this,
        SLOT(handleGdbStartFailed1()));
}

void RemotePlainGdbAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(QLatin1String("TRYING TO START ADAPTER"));

    if (!startParameters().workingDirectory.isEmpty())
        m_gdbProc.setWorkingDirectory(startParameters().workingDirectory);
    if (startParameters().environment.size())
        m_gdbProc.setEnvironment(startParameters().environment.toStringList());

    if (startParameters().requestRemoteSetup)
        m_engine->notifyEngineRequestRemoteSetup();
    else
        handleRemoteSetupDone(startParameters().connParams.port, startParameters().qmlServerPort);
}

void RemotePlainGdbAdapter::setupInferior()
{
    AbstractPlainGdbAdapter::setupInferior();
    m_engine->postCommand("directory "
        + m_engine->startParameters().remoteSourcesDir);
}

void RemotePlainGdbAdapter::interruptInferior()
{
    m_gdbProc.interruptInferior();
}

QByteArray RemotePlainGdbAdapter::execFilePath() const
{
    return startParameters().executable.toUtf8();
}

QByteArray RemotePlainGdbAdapter::toLocalEncoding(const QString &s) const
{
    return s.toUtf8();
}

QString RemotePlainGdbAdapter::fromLocalEncoding(const QByteArray &b) const
{
    return QString::fromUtf8(b);
}

void RemotePlainGdbAdapter::handleApplicationOutput(const QByteArray &output)
{
    showMessage(QString::fromUtf8(output), AppOutput);
}

void RemotePlainGdbAdapter::shutdownAdapter()
{
    m_engine->notifyAdapterShutdownOk();
}

void RemotePlainGdbAdapter::handleRemoteSetupDone(int gdbServerPort, int qmlPort)
{
    Q_UNUSED(gdbServerPort);
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());

    if (qmlPort != -1)
        startParameters().qmlServerPort = qmlPort;
    m_gdbProc.realStart(m_engine->startParameters().debuggerCommand,
        QStringList() << QLatin1String("-i") << QLatin1String("mi"),
        m_engine->startParameters().executable);
}

void RemotePlainGdbAdapter::handleGdbStarted()
{
     m_engine->startGdb();
}

void RemotePlainGdbAdapter::handleGdbStartDone()
{
     m_engine->handleAdapterStarted();
}

void RemotePlainGdbAdapter::handleGdbStartFailed()
{
}

void RemotePlainGdbAdapter::handleGdbStartFailed1()
{
    m_engine->handleAdapterStartFailed(m_gdbProc.errorString());
}

void RemotePlainGdbAdapter::handleRemoteSetupFailed(const QString &reason)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());

    m_engine->handleAdapterStartFailed(reason);
}

} // namespace Internal
} // namespace Debugger
