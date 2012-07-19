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

#include "codamessage.h"
#include "json.h"

#include <QString>
#include <QTextStream>

// Names matching the enum
static const char *serviceNamesC[] =
{ "Locator", "RunControl", "Processes", "Memory", "Settings", "Breakpoints",
  "Registers", "Logging", "FileSystem", "SymbianInstall", "SymbianOSData",
  "DebugSessionControl",
  "UnknownService"};

using namespace Json;
namespace Coda {

SYMBIANUTILS_EXPORT QString joinByteArrays(const QVector<QByteArray> &a, char sep)
{
    QString rc;
    const int count = a.size();
    for (int i = 0; i < count; i++) {
        if (i)
            rc += QLatin1Char(sep);
        rc += QString::fromUtf8(a.at(i));
    }
    return rc;
}

static inline bool jsonToBool(const JsonValue& js)
{
    return js.type() == JsonValue::Boolean && js.data() == "true";
}

SYMBIANUTILS_EXPORT const char *serviceName(Services s)
{
    return serviceNamesC[s];
}

SYMBIANUTILS_EXPORT Services serviceFromName(const char *n)
{
    const int count = sizeof(serviceNamesC)/sizeof(char *);
    for (int i = 0; i < count; i++)
        if (!qstrcmp(serviceNamesC[i], n))
            return static_cast<Services>(i);
    return UnknownService;
}

SYMBIANUTILS_EXPORT QString formatData(const QByteArray &a)
{
    const int columns = 16;
    QString rc;
    QTextStream str(&rc);
    str.setIntegerBase(16);
    str.setPadChar(QLatin1Char('0'));
    const unsigned char *start = reinterpret_cast<const unsigned char *>(a.constData());
    const unsigned char *end = start + a.size();
    for  (const unsigned char *p = start; p < end ; ) {
        str << "0x";
        str.setFieldWidth(4);
        str << (p - start);
        str.setFieldWidth(0);
        str << ' ';
        QString asc;
        int c = 0;
        for ( ; c < columns && p < end; c++, p++) {
            const unsigned u = *p;
            str.setFieldWidth(2);
            str << u;
            str.setFieldWidth(0);
            str << ' ';
            switch (u) {
            case '\n':
                asc += QLatin1String("\\n");
                break;
            case '\r':
                asc += QLatin1String("\\r");
                break;
            case '\t':
                asc += QLatin1String("\\t");
                break;
            default:
                if (u >= 32 && u < 128) {
                    asc += QLatin1Char(' ');
                    asc += QLatin1Char(u);
                } else {
                    asc += QLatin1String(" .");
                }
                break;
            }
        }
        if (const int remainder = columns - c)
            str << QString(3 * remainder, QLatin1Char(' '));
        str << ' ' << asc << '\n';
    }
    return rc;
}

// ----------- RunControlContext
RunControlContext::RunControlContext() :
        flags(0), resumeFlags(0)
{
}

void RunControlContext::clear()
{
    flags =0;
    resumeFlags = 0;
    id.clear();
    osid.clear();
    parentId.clear();
}

RunControlContext::Type RunControlContext::typeFromCodaId(const QByteArray &id)
{
    // "p12" or "p12.t34"?
    return id.contains(".t") ? Thread : Process;
}

unsigned RunControlContext::processId() const
{
    return processIdFromTcdfId(id);
}

unsigned RunControlContext::threadId() const
{
    return threadIdFromTcdfId(id);
}

unsigned RunControlContext::processIdFromTcdfId(const QByteArray &id)
{
    // Cut out process id from "p12" or "p12.t34"?
    if (!id.startsWith('p'))
        return 0;
    const int dotPos = id.indexOf('.');
    const int pLen = dotPos == -1 ? id.size() : dotPos;
    return id.mid(1, pLen - 1).toUInt();
}

unsigned RunControlContext::threadIdFromTcdfId(const QByteArray &id)
{
    const int tPos = id.indexOf(".t");
    return tPos != -1 ? id.mid(tPos + 2).toUInt() : uint(0);
}

QByteArray RunControlContext::codaId(unsigned processId,  unsigned threadId /* = 0 */)
{
    QByteArray rc("p");
    rc += QByteArray::number(processId);
    if (threadId) {
        rc += ".t";
        rc += QByteArray::number(threadId);
    }
    return rc;
}

RunControlContext::Type RunControlContext::type() const
{
    return RunControlContext::typeFromCodaId(id);
}

bool RunControlContext::parse(const JsonValue &val)
{
    clear();
    if (val.type() != JsonValue::Object)
        return false;
    foreach(const JsonValue &c, val.children()) {
        if (c.name() == "ID") {
            id = c.data();
        } else if (c.name() == "OSID") {
            osid = c.data();
        } else if (c.name() == "ParentID") {
            parentId = c.data();
        }  else if (c.name() == "IsContainer") {
            if (jsonToBool(c))
                flags |= Container;
        }  else if (c.name() == "CanTerminate") {
            if (jsonToBool(c))
                flags |= CanTerminate;
        }  else if (c.name() == "CanResume") {
            resumeFlags = c.data().toUInt();
        }  else if (c.name() == "HasState") {
            if (jsonToBool(c))
                flags |= HasState;
        } else if (c.name() == "CanSuspend") {
            if (jsonToBool(c))
                flags |= CanSuspend;
        }
    }
    return true;
}

QString RunControlContext::toString() const
{
    QString rc;
    QTextStream str(&rc);
    format(str);
    return rc;
}

void RunControlContext::format(QTextStream &str) const
{
    str << " id='" << id << "' osid='" << osid
        << "' parentId='" << parentId <<"' ";
    if (flags & Container)
        str << "[container] ";
    if (flags & HasState)
        str << "[has state] ";
    if (flags & CanSuspend)
        str << "[can suspend] ";
    if (flags & CanSuspend)
        str << "[can terminate] ";
    str.setIntegerBase(16);
    str << " resume_flags: 0x" <<  resumeFlags;
    str.setIntegerBase(10);
}

// ------ ModuleLoadEventInfo
ModuleLoadEventInfo::ModuleLoadEventInfo() :
   loaded(false), codeAddress(0), dataAddress(0), requireResume(false)
{
}

void ModuleLoadEventInfo::clear()
{
    loaded = requireResume = false;
    codeAddress = dataAddress =0;
}

bool ModuleLoadEventInfo::parse(const JsonValue &val)
{
    clear();
    if (val.type() != JsonValue::Object)
        return false;
    foreach(const JsonValue &c, val.children()) {
        if (c.name() == "Name") {
            name = c.data();
        } else if (c.name() == "File") {
            file = c.data();
        } else if (c.name() == "CodeAddress") {
            codeAddress = c.data().toULongLong();
        }  else if (c.name() == "DataAddress") {
            dataAddress = c.data().toULongLong();
        }  else if (c.name() == "Loaded") {
            loaded = jsonToBool(c);
        }  else if (c.name() == "RequireResume") {
            requireResume =jsonToBool(c);
        }
    }
    return true;
}
void ModuleLoadEventInfo::format(QTextStream &str) const
{
    str << "name='" << name << "' file='" << file << "' " <<
            (loaded ? "[loaded] " : "[not loaded] ");
    if (requireResume)
        str << "[requires resume] ";
    str.setIntegerBase(16);
    str  << " code: 0x" << codeAddress << " data: 0x" << dataAddress;
    str.setIntegerBase(10);
}

// ---------------------- Breakpoint

// Types matching enum
static const char *breakPointTypesC[] = {"Software", "Hardware", "Auto"};

Breakpoint::Breakpoint(quint64 loc) :
    type(Auto), enabled(true), ignoreCount(0), location(loc), size(1), thumb(true)
{
    if (loc)
        id = idFromLocation(location);
}

void Breakpoint::setContextId(unsigned processId, unsigned threadId)
{
    contextIds = QVector<QByteArray>(1, RunControlContext::codaId(processId, threadId));
}

QByteArray Breakpoint::idFromLocation(quint64 loc)
{
    return QByteArray("BP_0x") + QByteArray::number(loc, 16);
}

QString Breakpoint::toString() const
{
    QString rc;
    QTextStream str(&rc);
    str.setIntegerBase(16);
    str << "Breakpoint '" << id << "' "  << breakPointTypesC[type] << " for contexts '"
            << joinByteArrays(contextIds, ',') << "' at 0x" << location;
    str.setIntegerBase(10);
    str << " size " << size;
    if (enabled)
        str << " [enabled]";
    if (thumb)
        str << " [thumb]";
    if (ignoreCount)
        str << " IgnoreCount " << ignoreCount;
    return rc;
}

JsonInputStream &operator<<(JsonInputStream &str, const Breakpoint &b)
{
    if (b.contextIds.isEmpty())
        qWarning("Coda::Breakpoint: No context ids specified");

    str << '{' << "ID" << ':' << QString::fromUtf8(b.id) << ','
        << "BreakpointType" << ':' << breakPointTypesC[b.type] << ','
        << "Enabled" << ':' << b.enabled << ','
        << "IgnoreCount" << ':' << b.ignoreCount << ','
        << "ContextIds" << ':' << b.contextIds << ','
        << "Location" << ':' << QString::number(b.location) << ','
        << "Size"  << ':' << b.size << ','
        << "THUMB_BREAKPOINT" << ':' << b.thumb
        << '}';
    return str;
}

// --- Events
CodaEvent::CodaEvent(Type type) : m_type(type)
{
}

CodaEvent::~CodaEvent()
{
}

CodaEvent::Type CodaEvent::type() const
{
    return m_type;
}

QString CodaEvent::toString() const
{
    return QString();
}

static const char sharedLibrarySuspendReasonC[] = "Shared Library";

CodaEvent *CodaEvent::parseEvent(Services s, const QByteArray &nameBA, const QVector<JsonValue> &values)
{
    switch (s) {
    case LocatorService:
        if (nameBA == "Hello" && values.size() == 1 && values.front().type() == JsonValue::Array) {
            QStringList services;
            foreach (const JsonValue &jv, values.front().children())
                services.push_back(QString::fromUtf8(jv.data()));
            return new CodaLocatorHelloEvent(services);
        }
        break;
    case RunControlService:
        if (values.empty())
            return 0;
        // "id/PC/Reason/Data"
        if (nameBA == "contextSuspended" && values.size() == 4) {
            const QByteArray idBA = values.at(0).data();
            const quint64 pc = values.at(1).data().toULongLong();
            const QByteArray reasonBA = values.at(2).data();
            QByteArray messageBA;
            // Module load: Special
            if (reasonBA == sharedLibrarySuspendReasonC) {
                ModuleLoadEventInfo info;
                if (!info.parse(values.at(3)))
                    return 0;
                return new CodaRunControlModuleLoadContextSuspendedEvent(idBA, reasonBA, pc, info);
            } else {
                // hash containing a 'message'-key with a verbose crash message.
                if (values.at(3).type() == JsonValue::Object && values.at(3).childCount()
                    && values.at(3).children().at(0).type() == JsonValue::String)
                    messageBA = values.at(3).children().at(0).data();
            }
            return new CodaRunControlContextSuspendedEvent(idBA, reasonBA, messageBA, pc);
        } // "contextSuspended"
        if (nameBA == "contextAdded")
            return CodaRunControlContextAddedEvent::parseEvent(values);
        if (nameBA == "contextRemoved" && values.front().type() == JsonValue::Array) {
            QVector<QByteArray> ids;
            foreach(const JsonValue &c, values.front().children())
                ids.push_back(c.data());
            return new CodaRunControlContextRemovedEvent(ids);
        }
        break;
    case LoggingService:
        if ((nameBA == "writeln" || nameBA == "write" /*not yet used*/) && values.size() >= 2)
            return new CodaLoggingWriteEvent(values.at(0).data(), values.at(1).data());
        break;
    case ProcessesService:
        if (nameBA == "exited" && values.size() >= 2)
            return new CodaProcessExitedEvent(values.at(0).data());
        break;
   default:
        break;
    }
    return 0;
}

// -------------- CodaServiceHelloEvent
CodaLocatorHelloEvent::CodaLocatorHelloEvent(const QStringList &s) :
    CodaEvent(LocatorHello),
    m_services(s)
{
}

QString CodaLocatorHelloEvent::toString() const
{
    return QLatin1String("ServiceHello: ") + m_services.join(QLatin1String(", "));
}

// --------------  Logging event

CodaLoggingWriteEvent::CodaLoggingWriteEvent(const QByteArray &console, const QByteArray &message) :
    CodaEvent(LoggingWriteEvent), m_console(console), m_message(message)
{
}

QString CodaLoggingWriteEvent::toString() const
{
    return QString::fromUtf8(m_message);
}

// -------------- CodaIdEvent
CodaIdEvent::CodaIdEvent(Type t, const QByteArray &id) :
   CodaEvent(t), m_id(id)
{
}

// ---------- CodaIdsEvent
CodaIdsEvent::CodaIdsEvent(Type t, const QVector<QByteArray> &ids) :
    CodaEvent(t), m_ids(ids)
{
}

QString CodaIdsEvent::joinedIdString(const char sep) const
{
    return joinByteArrays(m_ids, sep);
}

//  ---------------- CodaRunControlContextAddedEvent
CodaRunControlContextAddedEvent::CodaRunControlContextAddedEvent(const RunControlContexts &c) :
        CodaEvent(RunControlContextAdded), m_contexts(c)
{
}

CodaRunControlContextAddedEvent
        *CodaRunControlContextAddedEvent::parseEvent(const QVector<JsonValue> &values)
{
    // Parse array of contexts
    if (values.size() < 1 || values.front().type() != JsonValue::Array)
        return 0;

    RunControlContexts contexts;
    foreach (const JsonValue &v, values.front().children()) {
        RunControlContext context;
        if (context.parse(v))
            contexts.push_back(context);
    }
    return new CodaRunControlContextAddedEvent(contexts);
}

QString CodaRunControlContextAddedEvent::toString() const
{
    QString rc;
    QTextStream str(&rc);
    str << "RunControl: " << m_contexts.size() << " context(s) "
        << (type() == RunControlContextAdded ? "added" : "removed")
        << '\n';
    foreach (const RunControlContext &c, m_contexts) {
        c.format(str);
        str << '\n';
    }
    return rc;
}

// --------------- CodaRunControlContextRemovedEvent
CodaRunControlContextRemovedEvent::CodaRunControlContextRemovedEvent(const QVector<QByteArray> &ids) :
        CodaIdsEvent(RunControlContextRemoved, ids)
{
}

QString CodaRunControlContextRemovedEvent::toString() const
{
    return QLatin1String("RunControl: Removed contexts '") + joinedIdString() + ("'.");
}

// --------------- CodaRunControlContextSuspendedEvent
CodaRunControlContextSuspendedEvent::CodaRunControlContextSuspendedEvent(const QByteArray &id,
                                                                             const QByteArray &reason,
                                                                             const QByteArray &message,
                                                                             quint64 pc) :
        CodaIdEvent(RunControlSuspended, id), m_pc(pc), m_reason(reason), m_message(message)
{
}

CodaRunControlContextSuspendedEvent::CodaRunControlContextSuspendedEvent(Type t,
                                                                             const QByteArray &id,
                                                                             const QByteArray &reason,
                                                                             quint64 pc) :
        CodaIdEvent(t, id), m_pc(pc), m_reason(reason)
{
}

void CodaRunControlContextSuspendedEvent::format(QTextStream &str) const
{
    str.setIntegerBase(16);
    str << "RunControl: '" << idString()  << "' suspended at 0x"
            << m_pc << ": '" << m_reason << "'.";
    str.setIntegerBase(10);
    if (!m_message.isEmpty())
        str << " (" <<m_message << ')';
}

QString CodaRunControlContextSuspendedEvent::toString() const
{
    QString rc;
    QTextStream str(&rc);
    format(str);
    return rc;
}

CodaRunControlContextSuspendedEvent::Reason CodaRunControlContextSuspendedEvent::reason() const
{
    if (m_reason == sharedLibrarySuspendReasonC)
        return ModuleLoad;
    if (m_reason == "Breakpoint")
        return BreakPoint;
    // 'Data abort exception'/'Thread has panicked' ... unfortunately somewhat unspecific.
    if (m_reason.contains("Exception") || m_reason.contains("panick"))
        return Crash;   
    return Other;
}

CodaRunControlModuleLoadContextSuspendedEvent::CodaRunControlModuleLoadContextSuspendedEvent(const QByteArray &id,
                                                                                                 const QByteArray &reason,
                                                                                                 quint64 pc,
                                                                                                 const ModuleLoadEventInfo &mi) :
    CodaRunControlContextSuspendedEvent(RunControlModuleLoadSuspended, id, reason, pc),
    m_mi(mi)
{
}

QString CodaRunControlModuleLoadContextSuspendedEvent::toString() const
{
    QString rc;
    QTextStream str(&rc);
    CodaRunControlContextSuspendedEvent::format(str);
    str <<  ' ';
    m_mi.format(str);
    return rc;
}

// -------------- CodaIdEvent
CodaProcessExitedEvent::CodaProcessExitedEvent(const QByteArray &id) :
   CodaEvent(ProcessExitedEvent), m_id(id)
{
}

QString CodaProcessExitedEvent::toString() const
{
    return QString("Process \"%1\" exited").arg(idString());
}

} // namespace Coda
