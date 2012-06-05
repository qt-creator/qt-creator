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

#include "localplaingdbadapter.h"

#include "gdbengine.h"
#include "debuggerstartparameters.h"
#include "procinterrupt.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"

#include <projectexplorer/abi.h>

#include <utils/qtcassert.h>

#include <QFileInfo>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// PlainGdbAdapter
//
///////////////////////////////////////////////////////////////////////

LocalPlainGdbAdapter::LocalPlainGdbAdapter(GdbEngine *engine)
    : AbstractPlainGdbAdapter(engine)
{
    // Output
    connect(&m_outputCollector, SIGNAL(byteDelivery(QByteArray)),
        engine, SLOT(readDebugeeOutput(QByteArray)));
}

AbstractGdbAdapter::DumperHandling LocalPlainGdbAdapter::dumperHandling() const
{
    // LD_PRELOAD fails for System-Qt on Mac.
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    return DumperLoadedByGdb;
#else
    return DumperLoadedByGdbPreload;
#endif
}

void LocalPlainGdbAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));

    if (!prepareCommand())
        return;

    QStringList gdbArgs;

    if (!m_outputCollector.listen()) {
        m_engine->handleAdapterStartFailed(tr("Cannot set up communication with child process: %1")
                .arg(m_outputCollector.errorString()), QString());
        return;
    }
    gdbArgs.append(_("--tty=") + m_outputCollector.serverName());

    if (!startParameters().workingDirectory.isEmpty())
        m_gdbProc.setWorkingDirectory(startParameters().workingDirectory);
    if (startParameters().environment.size())
        m_gdbProc.setEnvironment(startParameters().environment.toStringList());

    m_engine->startGdb(gdbArgs);
}

void LocalPlainGdbAdapter::handleGdbStartDone()
{
    m_engine->handleAdapterStarted();
}

void LocalPlainGdbAdapter::handleGdbStartFailed()
{
    m_outputCollector.shutdown();
}

void LocalPlainGdbAdapter::setupInferior()
{
    AbstractPlainGdbAdapter::setupInferior();
}

void LocalPlainGdbAdapter::runEngine()
{
    AbstractPlainGdbAdapter::runEngine();
}

void LocalPlainGdbAdapter::shutdownAdapter()
{
    showMessage(_("PLAIN ADAPTER SHUTDOWN %1").arg(state()));
    m_outputCollector.shutdown();
    m_engine->notifyAdapterShutdownOk();
}

void LocalPlainGdbAdapter::interruptInferior()
{
    interruptLocalInferior(m_engine->inferiorPid());
}

QByteArray LocalPlainGdbAdapter::execFilePath() const
{
    return QFileInfo(startParameters().executable)
            .absoluteFilePath().toLocal8Bit();
}

QByteArray LocalPlainGdbAdapter::toLocalEncoding(const QString &s) const
{
    return s.toLocal8Bit();
}

QString LocalPlainGdbAdapter::fromLocalEncoding(const QByteArray &b) const
{
    return QString::fromLocal8Bit(b);
}

} // namespace Internal
} // namespace Debugger
