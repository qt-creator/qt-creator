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

///////////////////////////////////////////////////////////////////////
//
// CoreGdbAdapter
//
///////////////////////////////////////////////////////////////////////

CoreGdbAdapter::CoreGdbAdapter(GdbEngine *engine, QObject *parent)
    : AbstractGdbAdapter(engine, parent)
{
}

void CoreGdbAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineStarting, qDebug() << state());
    setState(AdapterStarting);
    debugMessage(_("TRYING TO START ADAPTER"));

    if (!m_engine->startGdb())
        return;

    emit adapterStarted();
}

void CoreGdbAdapter::startInferior()
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    m_executable = startParameters().executable;
    if (m_executable.isEmpty()) {
#ifdef EXE_FROM_CORE
        // Extra round trip to get executable name from core file.
        // This is sometimes not the full name, so it can't be used
        // as the generic solution.

        m_round = 1;
        loadCoreFile();
#else
        showMessageBox(QMessageBox::Warning, tr("Error Loading Symbols"),
                       tr("No executable to load symbols from specified."));
#endif
        return;
    }
#ifdef EXE_FROM_CORE
    m_round = 2;
#endif
    loadExeAndSyms();
}

void CoreGdbAdapter::loadExeAndSyms()
{
    // Do that first, otherwise no symbols are loaded.
    QFileInfo fi(m_executable);
    m_engine->postCommand(_("-file-exec-and-symbols \"%1\"")
        .arg(fi.absoluteFilePath()), CB(handleFileExecAndSymbols));
}

void CoreGdbAdapter::handleFileExecAndSymbols(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        showStatusMessage(tr("Symbols found."));
    } else {
        QString msg = tr("Loading symbols from \"%1\" failed:\n").arg(m_executable)
            + __(response.data.findChild("msg").data());
        showMessageBox(QMessageBox::Warning, tr("Error Loading Symbols"), msg);
    }
    loadCoreFile();
}

void CoreGdbAdapter::loadCoreFile()
{
    // Quoting core name below fails in gdb 6.8-debian.
    QFileInfo fi(startParameters().coreFile);
    QString coreName = fi.absoluteFilePath();
    m_engine->postCommand(_("target core ") + coreName, CB(handleTargetCore));
}

void CoreGdbAdapter::handleTargetCore(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
#ifdef EXE_FROM_CORE
        if (m_round == 1) {
            m_round = 2;
            GdbMi console = response.data.findChild("consolestreamoutput");
            int pos1 = console.data().indexOf('`');
            int pos2 = console.data().indexOf('\'');
            if (pos1 != -1 && pos2 != -1) {
                m_executable = console.data().mid(pos1 + 1, pos2 - pos1 - 1);
                // Strip off command line arguments. FIXME: make robust.
                int idx = m_executable.indexOf(_c(' '));
                if (idx >= 0)
                    m_executable.truncate(idx);
                if (!m_executable.isEmpty()) {
                    m_executable = QFileInfo(startParameters().coreFile).absoluteDir()
                                   .absoluteFilePath(m_executable);
                    if (QFile::exists(m_executable)) {
                        // Finish extra round ...
                        showStatusMessage(tr("Attached to core temporarily."));
                        m_engine->postCommand(_("detach"));
                        // ... and retry.
                        loadExeAndSyms();
                        return;
                    }
                }
            }
            showMessageBox(QMessageBox::Warning, tr("Error Loading Symbols"),
                           tr("Unable to determine executable from core file."));
        }
#endif
        showStatusMessage(tr("Attached to core."));
        setState(InferiorUnrunnable);
        m_engine->updateAll();
    } else {
        QString msg = tr("Attach to core \"%1\" failed:\n").arg(startParameters().coreFile)
            + __(response.data.findChild("msg").data());
        emit inferiorStartFailed(msg);
    }
}

void CoreGdbAdapter::interruptInferior()
{
    // A core should never 'run'
    QTC_ASSERT(false, /**/);
}

} // namespace Internal
} // namespace Debugger
