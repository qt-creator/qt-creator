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

#include "coregdbadapter.h"

#include "debuggerstartparameters.h"
#include "debuggercore.h"
#include "debuggeractions.h"
#include "debuggerstringutils.h"
#include "gdbmi.h"
#include "gdbengine.h"

#include <utils/consoleprocess.h>
#include <utils/elfreader.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QTemporaryFile>

using namespace Utils;

namespace Debugger {
namespace Internal {

#define CB(callback) \
    static_cast<GdbEngine::AdapterCallback>(&CoreGdbAdapter::callback), \
    STRINGIFY(callback)

///////////////////////////////////////////////////////////////////////
//
// CoreGdbAdapter
//
///////////////////////////////////////////////////////////////////////

CoreGdbAdapter::CoreGdbAdapter(GdbEngine *engine)
    : AbstractGdbAdapter(engine)
{}

CoreGdbAdapter::~CoreGdbAdapter()
{
    if (false && !m_tempCoreName.isEmpty()) {
        QFile tmpFile(m_tempCoreName);
        tmpFile.remove();
    }
}

void CoreGdbAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));

    const DebuggerStartParameters &sp = startParameters();
    m_executable = sp.executable;
    QFileInfo fi(sp.coreFile);
    m_coreName = fi.absoluteFilePath();

    unpackCoreIfNeeded();
}

void CoreGdbAdapter::continueAdapterStart()
{
    if (m_executable.isEmpty()) {
        // Read executable from core.
        ElfReader reader(coreFileName());
        bool isCore = false;
        m_executable = reader.readCoreName(&isCore);

        if (!isCore) {
            showMessageBox(QMessageBox::Warning,
                tr("Error Loading Core File"),
                tr("The specified file does not appear to be a core file."));
            m_engine->notifyEngineSetupFailed();
            return;
        }

        // Strip off command line arguments. FIXME: make robust.
        int idx = m_executable.indexOf(QLatin1Char(' '));
        if (idx >= 0)
            m_executable.truncate(idx);
        if (m_executable.isEmpty()) {
            showMessageBox(QMessageBox::Warning,
                tr("Error Loading Symbols"),
                tr("No executable to load symbols from specified core."));
            m_engine->notifyEngineSetupFailed();
            return;
        }
    }
    m_engine->startGdb();
}

void CoreGdbAdapter::handleGdbStartFailed()
{
}

void CoreGdbAdapter::handleGdbStartDone()
{
    m_engine->handleAdapterStarted();
}

void CoreGdbAdapter::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    // Do that first, otherwise no symbols are loaded.
    QFileInfo fi(m_executable);
    const QByteArray sysroot = startParameters().sysroot.toLocal8Bit();
    QByteArray path = fi.absoluteFilePath().toLocal8Bit();
    if (!sysroot.isEmpty()) {
        m_engine->postCommand("set sysroot " + sysroot);
        // sysroot is not enough to correctly locate the sources, so explicitly
        // relocate the most likely place for the debug source
        m_engine->postCommand("set substitute-path /usr/src " + sysroot + "/usr/src");
    }
    m_engine->postCommand("-file-exec-and-symbols \"" + path + '"',
         CB(handleFileExecAndSymbols));
}

void CoreGdbAdapter::handleFileExecAndSymbols(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    QString core = coreFileName();
    if (response.resultClass == GdbResultDone) {
        showMessage(tr("Symbols found."), StatusBar);
        m_engine->postCommand("target core " + core.toLocal8Bit(),
            CB(handleTargetCore));
        return;
    }
    QString msg = tr("No symbols found in core file <i>%1</i>.")
        .arg(core);
    msg += _(" ");
    msg += tr("This can be caused by a path length limitation in the "
        "core file.");
    msg += _(" ");
    msg += tr("Try to specify the binary using the "
        "<i>Debug->Start Debugging->Attach to Core</i> dialog.");
    m_engine->notifyInferiorSetupFailed(msg);
}

void CoreGdbAdapter::handleTargetCore(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        // HACK: The namespace is not accessible in the initial run.
        m_engine->loadPythonDumpers();
        showMessage(tr("Attached to core."), StatusBar);
        m_engine->handleInferiorPrepared();
        // Due to the auto-solib-add off setting, we don't have any
        // symbols yet. Load them in order of importance.
        m_engine->reloadStack(true);
        m_engine->postCommand("info shared", CB(handleModulesList));
        return;
    }
    QString msg = tr("Attach to core \"%1\" failed:\n")
        .arg(startParameters().coreFile)
        + QString::fromLocal8Bit(response.data.findChild("msg").data());
    m_engine->notifyInferiorSetupFailed(msg);
}

void CoreGdbAdapter::handleModulesList(const GdbResponse &response)
{
    m_engine->handleModulesList(response);
    loadSymbolsForStack();
}

void CoreGdbAdapter::loadSymbolsForStack()
{
    m_engine->loadSymbolsForStack();
    QTimer::singleShot(1000, this, SLOT(loadAllSymbols()));
}

void CoreGdbAdapter::loadAllSymbols()
{
    m_engine->loadAllSymbols();
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
    QTC_CHECK(false);
}

void CoreGdbAdapter::shutdownAdapter()
{
    m_engine->notifyAdapterShutdownOk();
}

void CoreGdbAdapter::unpackCoreIfNeeded()
{
    if (!m_coreName.endsWith(QLatin1String(".lzo"))) {
        continueAdapterStart();
        return;
    }

    {
        QString pattern = QDir::tempPath() + QLatin1String("/tmpcore-XXXXXX");
        QTemporaryFile tmp(pattern, this);
        tmp.open();
        m_tempCoreName = tmp.fileName();
    }

    QProcess *process = new QProcess(this);
    process->setWorkingDirectory(QDir::tempPath());
    process->start("/usr/bin/lzop", QStringList() << "-o" << m_tempCoreName << "-x" << m_coreName);
    connect(process, SIGNAL(finished(int)), SLOT(continueAdapterStart()));
}

QString CoreGdbAdapter::coreFileName() const
{
    return m_tempCoreName.isEmpty() ? m_coreName : m_tempCoreName;
}

} // namespace Internal
} // namespace Debugger
