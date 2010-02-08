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

#ifndef DEBUGGER_ABSTRACT_GDB_ADAPTER
#define DEBUGGER_ABSTRACT_GDB_ADAPTER

#include <QtCore/QObject>
#include <QtCore/QProcess>

#include "gdbengine.h"

namespace Debugger {
namespace Internal {

// AbstractGdbAdapter is inherited by PlainGdbAdapter used for local
// debugging and TrkGdbAdapter used for on-device debugging.
// In the PlainGdbAdapter case it's just a wrapper around a QProcess running
// gdb, in the TrkGdbAdapter case it's the interface to the gdb process in
// the whole rfcomm/gdb/gdbserver combo.
class AbstractGdbAdapter : public QObject
{
    Q_OBJECT

public:
    enum  DumperHandling { DumperNotAvailable,
                           DumperLoadedByAdapter,
                           DumperLoadedByGdbPreload,
                           DumperLoadedByGdb };

    AbstractGdbAdapter(GdbEngine *engine, QObject *parent = 0);
    virtual ~AbstractGdbAdapter();

    virtual void write(const QByteArray &data);

    virtual void startAdapter() = 0;
    virtual void startInferior() = 0;
    virtual void startInferiorPhase2();
    virtual void interruptInferior() = 0;
    virtual void shutdown();
    virtual const char *inferiorShutdownCommand() const;

    virtual DumperHandling dumperHandling() const = 0;

    static QString msgGdbStopFailed(const QString &why);
    static QString msgInferiorStopFailed(const QString &why);
    static QString msgAttachedToStoppedInferior();
    static QString msgInferiorStarted();
    static QString msgInferiorRunning();
    static QString msgConnectRemoteServerFailed(const QString &why);

    // Trk specific stuff
    virtual bool isTrkAdapter() const;
    virtual void trkReloadRegisters() {}
    virtual void trkReloadThreads() {}

signals:
    void adapterStarted();

    // Something went wrong with the adapter *before* adapterStarted() was emitted.
    // Make sure to clean up everything before emitting this signal.
    void adapterStartFailed(const QString &msg, const QString &settingsIdHint);

    // Something went wrong with the adapter *after* adapterStarted() was emitted.
    // Make sure to clean up everything before emitting this signal.
    void adapterCrashed(const QString &msg);

    // This triggers the initial breakpoint synchronization and causes
    // startInferiorPhase2() being called once done.
    void inferiorPrepared();

    // The adapter is still running just fine, but it failed to acquire a debuggee.
    void inferiorStartFailed(const QString &msg);

protected:
    DebuggerState state() const
        { return m_engine->state(); }
    void setState(DebuggerState state)
        { m_engine->setState(state); }
    const DebuggerStartParameters &startParameters() const
        { return m_engine->startParameters(); }
    void debugMessage(const QString &msg) const
        { m_engine->debugMessage(msg); }
    void showStatusMessage(const QString &msg) const
        { m_engine->showStatusMessage(msg); }
    void showMessageBox(int icon, const QString &title, const QString &text) const
        { m_engine->showMessageBox(icon, title, text); }

    GdbEngine * const m_engine;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_ABSTRACT_GDB_ADAPTER
