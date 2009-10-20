/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "remotegdbadapter.h"
#include "debuggerstringutils.h"
#include "debuggeractions.h"
#include "gdbengine.h"

#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>

#include <QtCore/QFileInfo>
#include <QtGui/QMessageBox>

namespace Debugger {
namespace Internal {

#define CB(callback) \
    static_cast<GdbEngine::AdapterCallback>(&RemoteGdbAdapter::callback), \
    STRINGIFY(callback)

///////////////////////////////////////////////////////////////////////
//
// RemoteGdbAdapter
//
///////////////////////////////////////////////////////////////////////

RemoteGdbAdapter::RemoteGdbAdapter(GdbEngine *engine, QObject *parent)
    : AbstractGdbAdapter(engine, parent)
{
    connect(&m_uploadProc, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(uploadProcError(QProcess::ProcessError)));
    connect(&m_uploadProc, SIGNAL(readyReadStandardOutput()),
        this, SLOT(readUploadStandardOutput()));
    connect(&m_uploadProc, SIGNAL(readyReadStandardError()),
        this, SLOT(readUploadStandardError()));
}

void RemoteGdbAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineStarting, qDebug() << state());
    setState(AdapterStarting);
    debugMessage(_("TRYING TO START ADAPTER"));

    // FIXME: make asynchroneous
    // Start the remote server
    if (startParameters().serverStartScript.isEmpty()) {
        m_engine->showStatusMessage(_("No server start script given. "
            "Assuming server runs already."));
    } else {
        m_uploadProc.start(_("/bin/sh ") + startParameters().serverStartScript);
        m_uploadProc.waitForStarted();
    }

    if (!m_engine->startGdb())
        // FIXME: cleanup missing
        return;

    emit adapterStarted();
}

void RemoteGdbAdapter::uploadProcError(QProcess::ProcessError error)
{
    QString msg;
    switch (error) {
        case QProcess::FailedToStart:
            msg = tr("The upload process failed to start. Either the "
                "invoked script '%1' is missing, or you may have insufficient "
                "permissions to invoke the program.")
                .arg(theDebuggerStringSetting(GdbLocation));
            break;
        case QProcess::Crashed:
            msg = tr("The upload process crashed some time after starting "
                "successfully.");
            break;
        case QProcess::Timedout:
            msg = tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
            break;
        case QProcess::WriteError:
            msg = tr("An error occurred when attempting to write "
                "to the upload process. For example, the process may not be running, "
                "or it may have closed its input channel.");
            break;
        case QProcess::ReadError:
            msg = tr("An error occurred when attempting to read from "
                "the upload process. For example, the process may not be running.");
            break;
        default:
            msg = tr("An unknown error in the upload process occurred. "
                "This is the default return value of error().");
    }

    m_engine->showStatusMessage(msg);
    QMessageBox::critical(m_engine->mainWindow(), tr("Error"), msg);
}

void RemoteGdbAdapter::readUploadStandardOutput()
{
    QByteArray ba = m_uploadProc.readAllStandardOutput();
    m_engine->gdbOutputAvailable(LogOutput, QString::fromLocal8Bit(ba, ba.length()));
}

void RemoteGdbAdapter::readUploadStandardError()
{
    QByteArray ba = m_uploadProc.readAllStandardError();
    m_engine->gdbOutputAvailable(LogError, QString::fromLocal8Bit(ba, ba.length()));
}

void RemoteGdbAdapter::startInferior()
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());

    m_engine->postCommand(_("set architecture %1")
        .arg(startParameters().remoteArchitecture));

    if (!startParameters().processArgs.isEmpty())
        m_engine->postCommand(_("-exec-arguments ")
            + startParameters().processArgs.join(_(" ")));

#if 0
    m_engine->postCommand(_("set target-async on"), CB(handleSetTargetAsync));
#else
    QFileInfo fi(startParameters().executable);
    QString fileName = fi.absoluteFilePath();
    m_engine->postCommand(_("-file-exec-and-symbols \"%1\"").arg(fileName),
        CB(handleFileExecAndSymbols));
#endif
}

#if 0
void RemoteGdbAdapter::handleSetTargetAsync(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        //qq->breakHandler()->setAllPending();
        QFileInfo fi(startParameters().executable);
        QString fileName = fi.absoluteFilePath();
        m_engine->postCommand(_("-file-exec-and-symbols \"%1\"").arg(fileName),
            CB(handleFileExecAndSymbols));
    } else {
        QString msg = tr("Adapter too old: does not support asynchronous mode.");
        emit inferiorStartFailed(msg);
    }
}
#endif

void RemoteGdbAdapter::handleFileExecAndSymbols(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        //m_breakHandler->clearBreakMarkers();

        // "target remote" does three things:
        //     (1) connects to the gdb server
        //     (2) starts the remote application
        //     (3) stops the remote application (early, e.g. in the dynamic linker)
        QString channel = startParameters().remoteChannel;
        m_engine->postCommand(_("target remote %1").arg(channel),
            CB(handleTargetRemote));
    } else {
        QString msg = tr("Starting remote executable failed:\n");
        msg += __(response.data.findChild("msg").data());
        emit inferiorStartFailed(msg);
    }
}

void RemoteGdbAdapter::handleTargetRemote(const GdbResponse &record)
{
    if (record.resultClass == GdbResultDone) {
        QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
        // gdb server will stop the remote application itself.
        debugMessage(_("INFERIOR STARTED"));
        showStatusMessage(msgAttachedToStoppedInferior());
        emit inferiorPrepared();
    } else {
        // 16^error,msg="hd:5555: Connection timed out."
        QString msg = msgConnectRemoteServerFailed(__(record.data.findChild("msg").data()));
        emit inferiorStartFailed(msg);
    }
}

void RemoteGdbAdapter::startInferiorPhase2()
{
    m_engine->continueInferiorInternal();
}

void RemoteGdbAdapter::interruptInferior()
{
    m_engine->postCommand(_("-exec-interrupt"));
}

void RemoteGdbAdapter::shutdown()
{
    // FIXME: cleanup missing
}

} // namespace Internal
} // namespace Debugger
