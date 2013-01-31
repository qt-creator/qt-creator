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

#include "localplaingdbadapter.h"

#include "gdbengine.h"
#include "debuggerstartparameters.h"
#include "procinterrupt.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"

#include <projectexplorer/abi.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// PlainGdbAdapter
//
///////////////////////////////////////////////////////////////////////

GdbLocalPlainEngine::GdbLocalPlainEngine(const DebuggerStartParameters &startParameters)
    : GdbAbstractPlainEngine(startParameters)
{
    // Output
    connect(&m_outputCollector, SIGNAL(byteDelivery(QByteArray)),
        this, SLOT(readDebugeeOutput(QByteArray)));
}

GdbEngine::DumperHandling GdbLocalPlainEngine::dumperHandling() const
{
    // LD_PRELOAD fails for System-Qt on Mac.
    return Utils::HostOsInfo::isWindowsHost() || Utils::HostOsInfo::isMacHost()
            ? DumperLoadedByGdb : DumperLoadedByGdbPreload;
}

void GdbLocalPlainEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));

    if (!prepareCommand())
        return;

    QStringList gdbArgs;

    if (!m_outputCollector.listen()) {
        handleAdapterStartFailed(tr("Cannot set up communication with child process: %1")
                .arg(m_outputCollector.errorString()));
        return;
    }
    gdbArgs.append(_("--tty=") + m_outputCollector.serverName());

    if (!startParameters().workingDirectory.isEmpty())
        m_gdbProc.setWorkingDirectory(startParameters().workingDirectory);
    if (startParameters().environment.size())
        m_gdbProc.setEnvironment(startParameters().environment.toStringList());

    startGdb(gdbArgs);
}

void GdbLocalPlainEngine::handleGdbStartFailed()
{
    m_outputCollector.shutdown();
}

void GdbLocalPlainEngine::shutdownEngine()
{
    showMessage(_("PLAIN ADAPTER SHUTDOWN %1").arg(state()));
    m_outputCollector.shutdown();
    notifyAdapterShutdownOk();
}

void GdbLocalPlainEngine::interruptInferior2()
{
    interruptLocalInferior(inferiorPid());
}

QByteArray GdbLocalPlainEngine::execFilePath() const
{
    return QFileInfo(startParameters().executable)
            .absoluteFilePath().toLocal8Bit();
}

QByteArray GdbLocalPlainEngine::toLocalEncoding(const QString &s) const
{
    return s.toLocal8Bit();
}

QString GdbLocalPlainEngine::fromLocalEncoding(const QByteArray &b) const
{
    return QString::fromLocal8Bit(b);
}

} // namespace Internal
} // namespace Debugger
