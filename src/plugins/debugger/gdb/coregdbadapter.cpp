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

#include "coregdbadapter.h"

#include "debuggeractions.h"
#include "gdbengine.h"
#include "debuggerstringutils.h"

#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QMessageBox>

namespace Debugger {
namespace Internal {

#define CB(callback) \
    static_cast<GdbEngine::AdapterCallback>(&CoreGdbAdapter::callback), \
    STRINGIFY(callback)

#ifdef Q_OS_LINUX
# define EXE_FROM_CORE
#endif

///////////////////////////////////////////////////////////////////////
//
// CoreGdbAdapter
//
///////////////////////////////////////////////////////////////////////

static QByteArray coreName(const DebuggerStartParameters &sp)
{
    QFileInfo fi(sp.coreFile);
    return fi.absoluteFilePath().toLocal8Bit();
}

CoreGdbAdapter::CoreGdbAdapter(GdbEngine *engine, QObject *parent)
  : AbstractGdbAdapter(engine, parent),
    m_executable(startParameters().executable),
    m_coreName(coreName(startParameters()))
{}

void CoreGdbAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));

    if (!m_engine->startGdb())
        return;

#ifndef EXE_FROM_CORE
    if (m_executable.isEmpty()) {
        DebuggerEngine::showMessageBox(QMessageBox::Warning, tr("Error Loading Symbols"),
                       tr("No executable to load symbols from specified."));
    }
    m_engine->handleAdapterStarted();
#else
    // Extra round trip to get executable name from core file.
    // This is sometimes not the full name, so it can't be used
    // as the generic solution.

    // Quoting core name below fails in gdb 6.8-debian.
    m_engine->postCommand("target core " + m_coreName,
        CB(handleTemporaryTargetCore));
#endif
}

void CoreGdbAdapter::handleTemporaryTargetCore(const GdbResponse &response)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    if (response.resultClass != GdbResultDone) {
        showMessage(tr("Attach to core failed."), StatusBar);
        m_engine->notifyEngineSetupFailed();
        return;
    }

    GdbMi console = response.data.findChild("consolestreamoutput");
    int pos1 = console.data().indexOf('`');
    int pos2 = console.data().indexOf('\'');
    if (pos1 == -1 || pos2 == -1) {
        showMessage(tr("Attach to core failed."), StatusBar);
        m_engine->notifyEngineSetupFailed();
        return;
    }

    m_executable = console.data().mid(pos1 + 1, pos2 - pos1 - 1);
    // Strip off command line arguments. FIXME: make robust.
    int idx = m_executable.indexOf(_c(' '));
    if (idx >= 0)
        m_executable.truncate(idx);
    if (m_executable.isEmpty()) {
        m_engine->postCommand("detach");
        m_engine->notifyEngineSetupFailed();
        return;
    }
    m_executable = QFileInfo(startParameters().coreFile).absoluteDir()
                   .absoluteFilePath(m_executable);
    showMessage(tr("Attached to core temporarily."), StatusBar);
    m_engine->postCommand("detach", CB(handleTemporaryDetach));
}

void CoreGdbAdapter::handleTemporaryDetach(const GdbResponse &response)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    if (response.resultClass == GdbResultDone)
        m_engine->notifyEngineSetupOk();
    else
        m_engine->notifyEngineSetupFailed();
}

void CoreGdbAdapter::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    // Do that first, otherwise no symbols are loaded.
    QFileInfo fi(m_executable);
    QByteArray path = fi.absoluteFilePath().toLocal8Bit();
    m_engine->postCommand("-file-exec-and-symbols \"" + path + '"',
         CB(handleFileExecAndSymbols));
}

void CoreGdbAdapter::handleFileExecAndSymbols(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        showMessage(tr("Symbols found."), StatusBar);
        m_engine->postCommand("target core " + m_coreName,
            CB(handleTargetCore));
        return;
    }
    m_engine->notifyInferiorSetupFailed(_("File or symbols not found"));
}

void CoreGdbAdapter::handleTargetCore(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        // HACK: The namespace is not accessible in the initial run.
        m_engine->loadPythonDumpers();
        showMessage(tr("Attached to core."), StatusBar);
        m_engine->handleInferiorPrepared();
        return;
    }
    QString msg = tr("Attach to core \"%1\" failed:\n")
        .arg(startParameters().coreFile)
        + QString::fromLocal8Bit(response.data.findChild("msg").data());
    m_engine->notifyInferiorSetupFailed(msg);
}

void CoreGdbAdapter::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    m_engine->notifyInferiorUnrunnable();
    m_engine->updateAll();
}

void CoreGdbAdapter::interruptInferior()
{
    // A core never runs, so this cannot be called.
    QTC_ASSERT(false, /**/);
}

void CoreGdbAdapter::shutdownInferior()
{
    m_engine->notifyInferiorShutdownOk();
}

void CoreGdbAdapter::shutdownAdapter()
{
    m_engine->notifyAdapterShutdownOk();
}

} // namespace Internal
} // namespace Debugger
