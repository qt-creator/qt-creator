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

#ifndef DEBUGGER_ABSTRACT_GDB_ADAPTER
#define DEBUGGER_ABSTRACT_GDB_ADAPTER

#include <QtCore/QObject>
#include <QtCore/QProcess>

#include "gdbengine.h"

namespace Debugger {
namespace Internal {

class AbstractGdbProcess;

// AbstractGdbAdapter is inherited by PlainGdbAdapter used for local
// debugging and TrkGdbAdapter used for on-device debugging.
// In the PlainGdbAdapter case it's just a wrapper around a QProcess running
// gdb, in the TrkGdbAdapter case it's the interface to the gdb process in
// the whole rfcomm/gdb/gdbserver combo.
class AbstractGdbAdapter : public QObject
{
    Q_OBJECT

public:
    enum DumperHandling
    {
        DumperNotAvailable,
        DumperLoadedByAdapter,
        DumperLoadedByGdbPreload,
        DumperLoadedByGdb
    };

    AbstractGdbAdapter(GdbEngine *engine, QObject *parent = 0);
    virtual ~AbstractGdbAdapter();

    virtual void write(const QByteArray &data);

    virtual void startAdapter() = 0;
    virtual void setupInferior() = 0;
    virtual void runEngine() = 0;
    virtual void interruptInferior() = 0;
    virtual void shutdownInferior() = 0;
    virtual void shutdownAdapter() = 0;
    virtual AbstractGdbProcess *gdbProc() = 0;

    virtual DumperHandling dumperHandling() const = 0;

    static QString msgGdbStopFailed(const QString &why);
    static QString msgInferiorStopFailed(const QString &why);
    static QString msgAttachedToStoppedInferior();
    static QString msgInferiorSetupOk();
    static QString msgInferiorRunOk();
    static QString msgConnectRemoteServerFailed(const QString &why);

    // Trk specific stuff
    virtual bool isTrkAdapter() const;
    virtual void trkReloadRegisters() {}
    virtual void trkReloadThreads() {}

protected:
    DebuggerState state() const
        { return m_engine->state(); }
    const DebuggerStartParameters &startParameters() const
        { return m_engine->startParameters(); }
    void showMessage(const QString &msg, int channel = LogDebug, int timeout = 1)
        { m_engine->showMessage(msg, channel, timeout); }

    GdbEngine * const m_engine;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_ABSTRACT_GDB_ADAPTER
