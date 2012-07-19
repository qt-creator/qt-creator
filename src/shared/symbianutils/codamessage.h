/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef CODAMESSAGE_H
#define CODAMESSAGE_H

#include "symbianutils_global.h"
#include "json_global.h"

#include <QStringList>
#include <QVector>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace Json {
class JsonValue;
class JsonInputStream;
}

namespace Coda {

enum Services {
    LocatorService,
    RunControlService,
    ProcessesService,
    MemoryService,
    SettingsService,  // non-standard, CODA specific
    BreakpointsService,
    RegistersService,
    LoggingService,    // non-standard, CODA specific
    FileSystemService,
    SymbianInstallService,    // non-standard, CODA specific
    SymbianOSData,    // non-standard, CODA specific
    DebugSessionControl,    // non-standard, CODA specific
    UnknownService
}; // Note: Check string array 'serviceNamesC' of same size when modifying this.

// Modes of RunControl/'Resume' (see EDF documentation).
// As of 24.6.2010, RM_RESUME, RM_STEP_OVER, RM_STEP_INTO,
// RM_STEP_OVER_RANGE, RM_STEP_INTO_RANGE are supported with
// RANG_START/RANGE_END parameters.
enum RunControlResumeMode {
    RM_RESUME = 0,
    RM_STEP_OVER = 1, RM_STEP_INTO = 2,
    RM_STEP_OVER_LINE = 3, RM_STEP_INTO_LINE = 4,
    RM_STEP_OUT = 5, RM_REVERSE_RESUME = 6,
    RM_REVERSE_STEP_OVER = 7, RM_REVERSE_STEP_INTO = 8,
    RM_REVERSE_STEP_OVER_LINE = 9, RM_REVERSE_STEP_INTO_LINE = 10,
    RM_REVERSE_STEP_OUT = 11, RM_STEP_OVER_RANGE = 12,
    RM_STEP_INTO_RANGE = 13, RM_REVERSE_STEP_OVER_RANGE = 14,
    RM_REVERSE_STEP_INTO_RANGE = 15
};

SYMBIANUTILS_EXPORT const char *serviceName(Services s);
SYMBIANUTILS_EXPORT Services serviceFromName(const char *);

// Debug helpers
SYMBIANUTILS_EXPORT QString formatData(const QByteArray &a);
SYMBIANUTILS_EXPORT QString joinByteArrays(const QVector<QByteArray> &a, char sep = ',');

// Context used in 'RunControl contextAdded' events and in reply
// to 'Processes start'. Could be thread or process.
struct SYMBIANUTILS_EXPORT RunControlContext {
    enum Flags {
        Container = 0x1, HasState = 0x2, CanSuspend = 0x4,
        CanTerminate = 0x8
    };
    enum Type { Process, Thread };

    RunControlContext();
    Type type() const;
    unsigned processId() const;
    unsigned threadId() const;

    void clear();
    bool parse(const Json::JsonValue &v);
    void format(QTextStream &str) const;
    QString toString() const;

    // Helper for converting the CODA ids ("p12" or "p12.t34")
    static Type typeFromCodaId(const QByteArray &id);
    static unsigned processIdFromTcdfId(const QByteArray &id);
    static unsigned threadIdFromTcdfId(const QByteArray &id);
    static QByteArray codaId(unsigned processId,  unsigned threadId = 0);

    unsigned flags;
    unsigned resumeFlags;
    QByteArray id;     // "p434.t699"
    QByteArray osid;   // Non-standard: Process or thread id
    QByteArray parentId; // Parent process id of a thread.
};

// Module load information occurring with 'RunControl contextSuspended' events
struct SYMBIANUTILS_EXPORT ModuleLoadEventInfo {
    ModuleLoadEventInfo();
    void clear();
    bool parse(const Json::JsonValue &v);
    void format(QTextStream &str) const;

    QByteArray name;
    QByteArray file;
    bool loaded;
    quint64 codeAddress;
    quint64 dataAddress;
    bool requireResume;
};

// Breakpoint as supported by Coda source June 2010
// TODO: Add watchpoints,etc once they are implemented
struct SYMBIANUTILS_EXPORT Breakpoint {
    enum Type { Software, Hardware, Auto };

    explicit Breakpoint(quint64 loc = 0);
    void setContextId(unsigned processId, unsigned threadId = 0);
    QString toString() const;

    static QByteArray idFromLocation(quint64 loc); // Automagically determine from location

    Type type;
    bool enabled;
    int ignoreCount;
    QVector<QByteArray> contextIds;   // Process or thread ids.
    QByteArray id;                    // Id of the breakpoint;
    quint64 location;
    unsigned size;
    bool thumb;
};

Json::JsonInputStream &operator<<(Json::JsonInputStream &str, const Breakpoint &b);

// Event hierarchy
class SYMBIANUTILS_EXPORT CodaEvent
{
    Q_DISABLE_COPY(CodaEvent)

public:
    enum Type { None,
                LocatorHello,
                RunControlContextAdded,
                RunControlContextRemoved,
                RunControlSuspended,
                RunControlBreakpointSuspended,
                RunControlModuleLoadSuspended,
                RunControlResumed,
                LoggingWriteEvent, // Non-standard
                ProcessExitedEvent // Non-standard
              };

    virtual ~CodaEvent();

    Type type() const;
    virtual QString toString() const;

    static CodaEvent *parseEvent(Services s, const QByteArray &name, const QVector<Json::JsonValue> &val);

protected:
    explicit CodaEvent(Type type = None);

private:
    const Type m_type;
};

// ServiceHello
class SYMBIANUTILS_EXPORT CodaLocatorHelloEvent : public CodaEvent {
public:
    explicit CodaLocatorHelloEvent(const QStringList &);

    const QStringList &services() const { return m_services; }
    virtual QString toString() const;

private:
    QStringList m_services;
};

// Logging event (non-standard, CODA specific)
class SYMBIANUTILS_EXPORT CodaLoggingWriteEvent : public CodaEvent {
public:
    explicit CodaLoggingWriteEvent(const QByteArray &console, const QByteArray &message);

    QByteArray message() const { return m_message; }
    QByteArray console() const { return m_console; }

    virtual QString toString() const;

private:
    const QByteArray m_console;
    const QByteArray m_message;
};

// Base for events that just have one id as parameter
// (simple suspend)
class SYMBIANUTILS_EXPORT CodaIdEvent : public CodaEvent {
protected:
    explicit CodaIdEvent(Type t, const QByteArray &id);
public:
    QByteArray id() const { return m_id; }
    QString idString() const { return QString::fromUtf8(m_id); }

private:
    const QByteArray m_id;
};

// Base for events that just have some ids as parameter
// (context removed)
class SYMBIANUTILS_EXPORT CodaIdsEvent : public CodaEvent {
protected:
    explicit CodaIdsEvent(Type t, const QVector<QByteArray> &ids);

public:
    QVector<QByteArray> ids() const { return m_ids; }
    QString joinedIdString(const char sep = ',') const;

private:
    const QVector<QByteArray> m_ids;
};

// RunControlContextAdded
class SYMBIANUTILS_EXPORT CodaRunControlContextAddedEvent : public CodaEvent {
public:
    typedef QVector<RunControlContext> RunControlContexts;

    explicit CodaRunControlContextAddedEvent(const RunControlContexts &c);

    const RunControlContexts &contexts() const { return m_contexts; }
    virtual QString toString() const;

    static CodaRunControlContextAddedEvent *parseEvent(const QVector<Json::JsonValue> &val);

private:
    const RunControlContexts m_contexts;
};

// RunControlContextRemoved
class SYMBIANUTILS_EXPORT CodaRunControlContextRemovedEvent : public CodaIdsEvent {
public:
    explicit CodaRunControlContextRemovedEvent(const QVector<QByteArray> &id);
    virtual QString toString() const;
};

// Simple RunControlContextSuspended (process/thread)
class SYMBIANUTILS_EXPORT CodaRunControlContextSuspendedEvent : public CodaIdEvent {
public:
    enum Reason  { BreakPoint, ModuleLoad, Crash, Other } ;

    explicit CodaRunControlContextSuspendedEvent(const QByteArray &id,
                                                   const QByteArray &reason,
                                                   const QByteArray &message,
                                                   quint64 pc = 0);
    virtual QString toString() const;

    quint64 pc() const { return m_pc; }
    QByteArray reasonID() const { return m_reason; }
    Reason reason() const;
    QByteArray message() const { return m_message; }

protected:
    explicit CodaRunControlContextSuspendedEvent(Type t,
                                                   const QByteArray &id,
                                                   const QByteArray &reason,
                                                   quint64 pc = 0);
    void format(QTextStream &str) const;

private:
    const quint64 m_pc;
    const QByteArray m_reason;
    const QByteArray m_message;
};

// RunControlContextSuspended due to module load
class SYMBIANUTILS_EXPORT CodaRunControlModuleLoadContextSuspendedEvent : public CodaRunControlContextSuspendedEvent {
public:
    explicit CodaRunControlModuleLoadContextSuspendedEvent(const QByteArray &id,
                                                             const QByteArray &reason,
                                                             quint64 pc,
                                                             const ModuleLoadEventInfo &mi);

    virtual QString toString() const;
    const ModuleLoadEventInfo &info() const { return m_mi; }

private:
    const ModuleLoadEventInfo m_mi;
};

// Process exited event
class SYMBIANUTILS_EXPORT CodaProcessExitedEvent : public CodaEvent {
public:
    explicit CodaProcessExitedEvent(const QByteArray &id);

    QByteArray id() const { return m_id; }
    QString idString() const { return QString::fromUtf8(m_id); }
    virtual QString toString() const;

private:
    const QByteArray m_id;
};

} // namespace Coda
#endif // CODAMESSAGE_H
