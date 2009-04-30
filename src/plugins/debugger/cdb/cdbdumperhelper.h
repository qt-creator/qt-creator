/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CDBDUMPERHELPER_H
#define CDBDUMPERHELPER_H

#include "watchutils.h"
#include "cdbcom.h"

#include <QtCore/QStringList>
#include <QtCore/QMap>

namespace Debugger {
namespace Internal {

struct CdbComInterfaces;
class IDebuggerManagerAccessForEngines;
class DebuggerManager;

/* For code clarity, all the stuff related to custom dumpers goes here.
 * "Custom dumper" is a library compiled against the current
 * Qt containing functions to evaluate values of Qt classes
 * (such as QString, taking pointers to their addresses).
 * The library must be loaded into the debuggee.
 * Loading the dumpers requires making the debuggee call functions
 * (LoadLibrary() and the dumper functions). This only works if the
 * debuggee is in a 'well-defined' breakpoint state (such as at 'main()').
 * Calling the load functions from an IDebugEvent callback causes
 * WaitForEvent() to fail with unknown errors. Calling the load functions from an
 * non 'well-defined' (arbitrary) breakpoint state will cause LoadLibrary
 * to trigger an access violations.
 * Currently, we call the load function when stopping at 'main()' for which
 * we set a temporary break point if the user does not want to stop there. */

class CdbDumperHelper
{
    Q_DISABLE_COPY(CdbDumperHelper)
public:
   enum State {
        Disabled,
        NotLoaded,
        Loaded,
        Failed
    };

    explicit CdbDumperHelper(IDebuggerManagerAccessForEngines *access,
                             CdbComInterfaces *cif);
    ~CdbDumperHelper();

    State state() const { return m_state; }
    operator bool() const { return m_state == Loaded; }

    // Call before starting the debugger
    void reset(const QString &library, bool enabled);

    // Call in a temporary breakpoint state to actually load.
    void load(DebuggerManager *manager);

    enum DumpResult { DumpNotHandled, DumpOk, DumpError };
    DumpResult dumpType(const WatchData &d, bool dumpChildren, int source,
                        QList<WatchData> *result, QString *errorMessage);

    // bool handlesType(const QString &typeName) const;

    inline CdbComInterfaces *comInterfaces() const { return m_cif; }

private:
    void clearBuffer();
    bool resolveSymbols(QString *errorMessage);
    bool getKnownTypes(QString *errorMessage);
    bool getTypeSize(const QString &typeName, int *size, QString *errorMessage);
    bool runTypeSizeQuery(const QString &typeName, int *size, QString *errorMessage);
    bool callDumper(const QString &call, const QByteArray &inBuffer, const char **outputPtr, QString *errorMessage);

    enum DumpExecuteResult { DumpExecuteOk, DumpExecuteSizeFailed, DumpExecuteCallFailed };
    DumpExecuteResult executeDump(const WatchData &wd,
                                  const QtDumperHelper::TypeData& td, bool dumpChildren, int source,
                                  QList<WatchData> *result, QString *errorMessage);

    static bool writeToDebuggee(CIDebugDataSpaces *ds, const QByteArray &buffer, quint64 address, QString *errorMessage);

    const QString m_messagePrefix;
    State m_state;
    IDebuggerManagerAccessForEngines *m_access;
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
};

} // namespace Internal
} // namespace Debugger

#endif // CDBDUMPERHELPER_H
