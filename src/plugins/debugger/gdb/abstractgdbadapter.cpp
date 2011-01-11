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

#include "abstractgdbadapter.h"
#include "gdbengine.h"
#include "debuggerstartparameters.h"
#include "abstractgdbprocess.h"

#include <utils/qtcassert.h>
#ifdef Q_OS_WIN
#include <utils/qtcprocess.h>
#endif

#include <QtCore/QProcess>

namespace Debugger {
namespace Internal {

AbstractGdbAdapter::AbstractGdbAdapter(GdbEngine *engine, QObject *parent)
  : QObject(parent), m_engine(engine)
{
}

AbstractGdbAdapter::~AbstractGdbAdapter()
{
    disconnect();
}

//void AbstractGdbAdapter::shutdown()
//{
//}

//void AbstractGdbAdapter::runEngine()
//{
//    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
//}

/*
const char *AbstractGdbAdapter::inferiorShutdownCommand() const
{
    return "kill";
}
*/

void AbstractGdbAdapter::write(const QByteArray &data)
{
    gdbProc()->write(data);
}

bool AbstractGdbAdapter::isTrkAdapter() const
{
    return false;
}

#ifdef Q_OS_WIN
bool AbstractGdbAdapter::prepareWinCommand()
{
    Utils::QtcProcess::SplitError perr;
    startParameters().processArgs = Utils::QtcProcess::prepareArgs(
                startParameters().processArgs, &perr,
                &startParameters().environment, &startParameters().workingDirectory);
    if (perr != Utils::QtcProcess::SplitOk) {
        // perr == BadQuoting is never returned on Windows
        // FIXME? QTCREATORBUG-2809
        m_engine->handleAdapterStartFailed(QCoreApplication::translate("DebuggerEngine", // Same message in CdbEngine
            "Debugging complex command lines is currently not supported under Windows"), QString());
        return false;
    }
    return true;
}
#endif

QString AbstractGdbAdapter::msgGdbStopFailed(const QString &why)
{
    return tr("The Gdb process could not be stopped:\n%1").arg(why);
}

QString AbstractGdbAdapter::msgInferiorStopFailed(const QString &why)
{
    return tr("Application process could not be stopped:\n%1").arg(why);
}

QString AbstractGdbAdapter::msgInferiorSetupOk()
{
    return tr("Application started");
}

QString AbstractGdbAdapter::msgInferiorRunOk()
{
    return tr("Application running");
}

QString AbstractGdbAdapter::msgAttachedToStoppedInferior()
{
    return tr("Attached to stopped application");
}

QString AbstractGdbAdapter::msgConnectRemoteServerFailed(const QString &why)
{
    return tr("Connecting to remote server failed:\n%1").arg(why);
}

DebuggerState AbstractGdbAdapter::state() const
{
    return m_engine->state();
}

const DebuggerStartParameters &AbstractGdbAdapter::startParameters() const
{
    return m_engine->startParameters();
}

DebuggerStartParameters &AbstractGdbAdapter::startParameters()
{
    return m_engine->startParameters();
}

void AbstractGdbAdapter::showMessage(const QString &msg, int channel, int timeout)
{
    m_engine->showMessage(msg, channel, timeout);
}

void AbstractGdbAdapter::handleRemoteSetupDone(int gdbServerPort, int qmlPort)
{
    Q_UNUSED(gdbServerPort);
    Q_UNUSED(qmlPort);
}

void AbstractGdbAdapter::handleRemoteSetupFailed(const QString &reason)
{
    Q_UNUSED(reason);
}

} // namespace Internal
} // namespace Debugger
