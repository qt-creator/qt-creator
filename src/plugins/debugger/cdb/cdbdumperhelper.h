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

#ifndef CDBDUMPERHELPER_H
#define CDBDUMPERHELPER_H

#include "watchutils.h"
#include "cdbcom.h"

#include <QtCore/QStringList>
#include <QtCore/QMap>

namespace Debugger {
class DebuggerManager;

namespace Internal {
struct CdbComInterfaces;

/* For code clarity, all the stuff related to custom dumpers goes here.
 * "Custom dumper" is a library compiled against the current
 * Qt containing functions to evaluate values of Qt classes
 * (such as QString), taking pointers to their addresses.
 * The dumper functions produce formatted string output which is
 * converted into WatchData items with the help of QtDumperHelper.
 *
 * Usage: When launching the debugger, call reset() with path to dumpers
 * and enabled flag. From the module load event callback, call
 * moduleLoadHook() to initialize.
 * dumpType() is the main query function to obtain a list  of WatchData from
 * WatchData item produced by the smbol context.
 * Call disable(), should the debuggee crash (as performing debuggee
 * calls is no longer possible, then).
 *
 * dumperCallThread specifies the thread to use when making the calls.
 * As of Debugging Tools v 6.11.1.404 (6.10.2009), calls cannot be executed
 * when the current thread is in some WaitFor...() function. The call will
 * then hang (regardless whether that thread or some other, non-blocking thread
 * is used), and the debuggee will be in running state afterwards (causing errors
 * from ReadVirtual, etc).
 * The current thread can be used when stepping or a breakpoint was
 * hit. When interrupting the inferior, an artifical thread is created,
 * that is not usable, either. */

class CdbDumperHelper
{
    Q_DISABLE_COPY(CdbDumperHelper)
public:
   enum State {
        Disabled, // Disabled or failed
        NotLoaded,
        InjectLoadFailed,
        InjectLoading,
        Loaded,
        Initialized, // List of types, etc. retrieved
    };

    explicit CdbDumperHelper(DebuggerManager *manager,
                             CdbComInterfaces *cif);
    ~CdbDumperHelper();

    State state() const    { return m_state; }
    bool isEnabled() const { return m_state != Disabled; }

    // Disable in case of a debuggee crash.
    void disable();

    // Call before starting the debugger
    void reset(const QString &library, bool enabled);

    // Call from the module load callback to perform initialization.
    void moduleLoadHook(const QString &module, HANDLE debuggeeHandle);

    // Dump a WatchData item.
    enum DumpResult { DumpNotHandled, DumpOk, DumpError };
    DumpResult dumpType(const WatchData &d, bool dumpChildren,
                        QList<WatchData> *result, QString *errorMessage);

    inline CdbComInterfaces *comInterfaces() const { return m_cif; }

    enum { InvalidDumperCallThread = 0xFFFFFFFF };
    unsigned long dumperCallThread();
    void setDumperCallThread(unsigned long t);

private:
    enum CallLoadResult { CallLoadOk, CallLoadError, CallLoadNoQtApp, CallLoadAlreadyLoaded };

    void clearBuffer();

    bool ensureInitialized(QString *errorMessage);
    CallLoadResult initCallLoad(QString *errorMessage);
    bool initResolveSymbols(QString *errorMessage);
    bool initKnownTypes(QString *errorMessage);

    inline DumpResult dumpTypeI(const WatchData &d, bool dumpChildren,
                                QList<WatchData> *result, QString *errorMessage);

    bool getTypeSize(const QString &typeName, int *size, QString *errorMessage);
    bool runTypeSizeQuery(const QString &typeName, int *size, QString *errorMessage);
    bool callDumper(const QString &call, const QByteArray &inBuffer, const char **outputPtr,
                    bool ignoreAccessViolation, QString *errorMessage);

    enum DumpExecuteResult { DumpExecuteOk, DumpExecuteSizeFailed,
                             DumpComplexExpressionEncountered,
                             DumpExecuteCallFailed };
    DumpExecuteResult executeDump(const WatchData &wd,
                                  const QtDumperHelper::TypeData& td, bool dumpChildren,
                                  QList<WatchData> *result, QString *errorMessage);

    static bool writeToDebuggee(CIDebugDataSpaces *ds, const QByteArray &buffer, quint64 address, QString *errorMessage);

    const bool m_tryInjectLoad;
    State m_state;
    DebuggerManager *m_manager;
    CdbComInterfaces *m_cif;

    QString m_library;
    QString m_dumpObjectSymbol;

    quint64 m_inBufferAddress;
    unsigned long m_inBufferSize;
    quint64 m_outBufferAddress;
    unsigned long m_outBufferSize;
    char *m_buffer;

    QStringList m_failedTypes;

    QtDumperHelper m_helper;
    unsigned long m_dumperCallThread;
    QString m_goCommand;
};

} // namespace Internal
} // namespace Debugger

#endif // CDBDUMPERHELPER_H
