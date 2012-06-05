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

#ifndef DEBUGGER_ABSTRACT_GDB_ADAPTER
#define DEBUGGER_ABSTRACT_GDB_ADAPTER

#include "debuggerconstants.h"
// Need to include gdbengine.h as otherwise MSVC crashes
// on invoking the first adapter callback in a *derived* adapter class.
#include "gdbengine.h"

#include <QObject>

namespace Debugger {
class DebuggerStartParameters;

namespace Internal {

class AbstractGdbProcess;
class GdbResponse;

// AbstractGdbAdapter is inherited by PlainGdbAdapter used for local
// debugging and CodaGdbAdapter used for on-device debugging.
// In the PlainGdbAdapter case it's just a wrapper around a QProcess running
// gdb, in the CodaGdbAdapter case it's the interface to the gdb process in
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

    explicit AbstractGdbAdapter(GdbEngine *engine);
    virtual ~AbstractGdbAdapter();

    virtual void write(const QByteArray &data);

    virtual void startAdapter() = 0;
    virtual void handleGdbStartDone() = 0;
    virtual void handleGdbStartFailed() = 0;
    virtual void setupInferior() = 0;
    virtual void runEngine() = 0;
    virtual void interruptInferior() = 0;
    virtual void shutdownAdapter() = 0;
    virtual AbstractGdbProcess *gdbProc() = 0;

    virtual DumperHandling dumperHandling() const = 0;

    static QString msgGdbStopFailed(const QString &why);
    static QString msgInferiorStopFailed(const QString &why);
    static QString msgAttachedToStoppedInferior();
    static QString msgInferiorSetupOk();
    static QString msgInferiorRunOk();
    static QString msgConnectRemoteServerFailed(const QString &why);

    // CODA specific stuff
    virtual bool isCodaAdapter() const;
    virtual void codaReloadRegisters() {}
    virtual void codaReloadThreads() {}

    virtual void handleRemoteSetupDone(int gdbServerPort, int qmlPort);
    virtual void handleRemoteSetupFailed(const QString &reason);


protected:
    DebuggerState state() const;
    GdbEngine *engine() const { return m_engine; }
    const DebuggerStartParameters &startParameters() const;
    DebuggerStartParameters &startParameters();
    void showMessage(const QString &msg, int channel = LogDebug, int timeout = 1);
    bool prepareCommand();
    void interruptLocalInferior(qint64 pid);

    GdbEngine * const m_engine;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_ABSTRACT_GDB_ADAPTER
