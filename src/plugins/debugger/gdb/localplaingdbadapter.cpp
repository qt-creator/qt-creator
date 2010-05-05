/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "localplaingdbadapter.h"

#include "gdbengine.h"
#include "procinterrupt.h"
#include "debuggerstringutils.h"

#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// PlainGdbAdapter
//
///////////////////////////////////////////////////////////////////////

LocalPlainGdbAdapter::LocalPlainGdbAdapter(GdbEngine *engine, QObject *parent)
    : AbstractPlainGdbAdapter(engine, parent)
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
    QTC_ASSERT(state() == EngineStarting, qDebug() << state());
    setState(AdapterStarting);
    debugMessage(_("TRYING TO START ADAPTER"));

    QStringList gdbArgs;

    if (!m_outputCollector.listen()) {
        emit adapterStartFailed(tr("Cannot set up communication with child process: %1")
                .arg(m_outputCollector.errorString()), QString());
        return;
    }
    gdbArgs.append(_("--tty=") + m_outputCollector.serverName());

    if (!startParameters().workingDir.isEmpty())
        m_gdbProc.setWorkingDirectory(startParameters().workingDir);
    if (!startParameters().environment.isEmpty())
        m_gdbProc.setEnvironment(startParameters().environment);

    if (!m_engine->startGdb(gdbArgs)) {
        m_outputCollector.shutdown();
        return;
    }

    emit adapterStarted();
}

void LocalPlainGdbAdapter::interruptInferior()
{
    const qint64 attachedPID = m_engine->inferiorPid();
    if (attachedPID <= 0) {
        debugMessage(_("TRYING TO INTERRUPT INFERIOR BEFORE PID WAS OBTAINED"));
        return;
    }

    if (!interruptProcess(attachedPID))
        debugMessage(_("CANNOT INTERRUPT %1").arg(attachedPID));
}

void LocalPlainGdbAdapter::shutdown()
{
    debugMessage(_("PLAIN ADAPTER SHUTDOWN %1").arg(state()));
    m_outputCollector.shutdown();
}

QByteArray LocalPlainGdbAdapter::execFilePath() const
{
    return QFileInfo(startParameters().executable)
            .absoluteFilePath().toLocal8Bit();
}

bool LocalPlainGdbAdapter::infoTargetNecessary() const
{
#ifdef Q_OS_LINUX
    return true;
#else
    return false;
#endif
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
