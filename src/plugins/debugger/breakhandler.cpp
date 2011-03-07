/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "breakhandler.h"
#include "breakpointmarker.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerstringutils.h"
#include "stackframe.h"

#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTimerEvent>


//////////////////////////////////////////////////////////////////
//
// BreakHandler
//
//////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

static QString stateToString(BreakpointState state)
{
    switch (state) {
        case BreakpointNew:
            return BreakHandler::tr("New");
        case BreakpointInsertRequested:
            return BreakHandler::tr("Insertion requested");
        case BreakpointInsertProceeding:
            return BreakHandler::tr("Insertion proceeding");
        case BreakpointChangeRequested:
            return BreakHandler::tr("Change requested");
        case BreakpointChangeProceeding:
            return BreakHandler::tr("Change proceeding");
        case BreakpointInserted:
            return BreakHandler::tr("Breakpoint inserted");
        case BreakpointRemoveRequested:
            return BreakHandler::tr("Removal requested");
        case BreakpointRemoveProceeding:
            return BreakHandler::tr("Removal proceeding");
        case BreakpointDead:
            return BreakHandler::tr("Dead");
        default:
            break;
    }
    //: Invalid breakpoint state.
    return BreakHandler::tr("<invalid state>");
}

static QString msgBreakpointAtSpecialFunc(const char *func)
{
    return BreakHandler::tr("Breakpoint at \"%1\"").arg(QString::fromAscii(func));
}

static QString typeToString(BreakpointType type)
{
    switch (type) {
        case BreakpointByFileAndLine:
            return BreakHandler::tr("Breakpoint by File and Line");
        case BreakpointByFunction:
            return BreakHandler::tr("Breakpoint by Function");
        case BreakpointByAddress:
            return BreakHandler::tr("Breakpoint by Address");
        case BreakpointAtThrow:
            return msgBreakpointAtSpecialFunc("throw");
        case BreakpointAtCatch:
            return msgBreakpointAtSpecialFunc("catch");
        case BreakpointAtFork:
            return msgBreakpointAtSpecialFunc("fork");
        case BreakpointAtExec:
            return msgBreakpointAtSpecialFunc("exec");
        case BreakpointAtVFork:
            return msgBreakpointAtSpecialFunc("vfork");
        case BreakpointAtSysCall:
            return msgBreakpointAtSpecialFunc("syscall");
        case BreakpointAtMain:
            return BreakHandler::tr("Breakpoint at Function \"main()\"");
        case Watchpoint:
            return BreakHandler::tr("Watchpoint");
        case UnknownType:
            break;
    }
    return BreakHandler::tr("Unknown Breakpoint Type");
}

BreakHandler::BreakHandler()
  : m_syncTimerId(-1)
{}

BreakHandler::~BreakHandler()
{}

QIcon BreakHandler::breakpointIcon()
{
    static QIcon icon(_(":/debugger/images/breakpoint_16.png"));
    return icon;
}

QIcon BreakHandler::disabledBreakpointIcon()
{
    static QIcon icon(_(":/debugger/images/breakpoint_disabled_16.png"));
    return icon;
}

QIcon BreakHandler::pendingBreakpointIcon()
{
    static QIcon icon(_(":/debugger/images/breakpoint_pending_16.png"));
    return icon;
}

QIcon BreakHandler::watchpointIcon()
{
    static QIcon icon(_(":/debugger/images/watchpoint.png"));
    return icon;
}

QIcon BreakHandler::tracepointIcon()
{
    static QIcon icon(_(":/debugger/images/tracepoint.png"));
    return icon;
}

QIcon BreakHandler::emptyIcon()
{
    static QIcon icon(_(":/debugger/images/breakpoint_pending_16.png"));
    //static QIcon icon(_(":/debugger/images/watchpoint.png"));
    //static QIcon icon(_(":/debugger/images/debugger_empty_14.png"));
    return icon;
}

int BreakHandler::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 8;
}

int BreakHandler::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_storage.size();
}

static inline bool fileNameMatch(const QString &f1, const QString &f2)
{
#ifdef Q_OS_WIN
    return f1.compare(f2, Qt::CaseInsensitive) == 0;
#else
    return f1 == f2;
#endif
}

static bool isSimilarTo(const BreakpointParameters &data, const BreakpointResponse &needle)
{
    // Clear hit.
    // Clear miss.
    if (needle.type != UnknownType && data.type != UnknownType
            && data.type != needle.type)
        return false;

    // Clear hit.
    if (data.address && data.address == needle.address)
        return true;

    // At least at a position we were looking for.
    // FIXME: breaks multiple breakpoints at the same location
    if (!data.fileName.isEmpty()
            && fileNameMatch(data.fileName, needle.fileName)
            && data.lineNumber == needle.lineNumber)
        return true;

    // At least at a position we were looking for.
    // FIXME: breaks multiple breakpoints at the same location
    if (!data.fileName.isEmpty()
            && fileNameMatch(data.fileName, needle.fileName)
            && data.lineNumber == needle.lineNumber)
        return true;

    return false;
}

BreakpointId BreakHandler::findSimilarBreakpoint(const BreakpointResponse &needle) const
{
    // Search a breakpoint we might refer to.
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it) {
        const BreakpointId id = it.key();
        const BreakpointParameters &data = it->data;
        const BreakpointResponse &response = it->response;
        //qDebug() << "COMPARING " << data.toString() << " WITH " << needle.toString();
        if (response.number && response.number == needle.number)
            return id;

        if (isSimilarTo(data, needle))
            return id;
    }
    return BreakpointId();
}

BreakpointId BreakHandler::findBreakpointByNumber(int bpNumber) const
{
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it)
        if (it->response.number == bpNumber)
            return it.key();
    return BreakpointId();
}

BreakpointId BreakHandler::findBreakpointByFunction(const QString &functionName) const
{
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it)
        if (it->data.functionName == functionName)
            return it.key();
    return BreakpointId();
}

BreakpointId BreakHandler::findBreakpointByAddress(quint64 address) const
{
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it)
        if (it->data.address == address || it->response.address == address)
            return it.key();
    return BreakpointId();
}

BreakpointId BreakHandler::findBreakpointByFileAndLine(const QString &fileName,
    int lineNumber, bool useMarkerPosition)
{
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it)
        if (it->isLocatedAt(fileName, lineNumber, useMarkerPosition))
            return it.key();
    return BreakpointId();
}

const BreakpointParameters &BreakHandler::breakpointData(BreakpointId id) const
{
    static BreakpointParameters dummy;
    ConstIterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return dummy);
    return it->data;
}

BreakpointId BreakHandler::findWatchpoint(const BreakpointParameters &data) const
{
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it)
        if (it->data.isWatchpoint()
                && it->data.address == data.address
                && it->data.size == data.size
                && it->data.bitpos == data.bitpos)
            return it.key();
    return BreakpointId();
}

void BreakHandler::saveBreakpoints()
{
    const QString one = _("1");
    //qDebug() << "SAVING BREAKPOINTS...";
    QTC_ASSERT(debuggerCore(), return);
    QList<QVariant> list;
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it) {
        const BreakpointParameters &data = it->data;
        QMap<QString, QVariant> map;
        // Do not persist Watchpoints.
        if (data.isWatchpoint())
            continue;
        if (data.type != BreakpointByFileAndLine)
            map.insert(_("type"), data.type);
        if (!data.fileName.isEmpty())
            map.insert(_("filename"), data.fileName);
        if (data.lineNumber)
            map.insert(_("linenumber"), data.lineNumber);
        if (!data.functionName.isEmpty())
            map.insert(_("funcname"), data.functionName);
        if (data.address)
            map.insert(_("address"), data.address);
        if (!data.condition.isEmpty())
            map.insert(_("condition"), data.condition);
        if (data.ignoreCount)
            map.insert(_("ignorecount"), data.ignoreCount);
        if (data.threadSpec >= 0)
            map.insert(_("threadspec"), data.threadSpec);
        if (!data.enabled)
            map.insert(_("disabled"), one);
        if (data.pathUsage != BreakpointPathUsageEngineDefault)
            map.insert(_("usefullpath"), QString::number(data.pathUsage));
        if (data.tracepoint)
            map.insert(_("tracepoint"), one);
        if (!data.module.isEmpty())
            map.insert(_("module"), data.module);
        if (!data.command.isEmpty())
            map.insert(_("command"), data.command);
        list.append(map);
    }
    debuggerCore()->setSessionValue("Breakpoints", list);
    //qDebug() << "SAVED BREAKPOINTS" << this << list.size();
}

void BreakHandler::loadBreakpoints()
{
    QTC_ASSERT(debuggerCore(), return);
    //qDebug() << "LOADING BREAKPOINTS...";
    QVariant value = debuggerCore()->sessionValue("Breakpoints");
    QList<QVariant> list = value.toList();
    //clear();
    foreach (const QVariant &var, list) {
        const QMap<QString, QVariant> map = var.toMap();
        BreakpointParameters data(BreakpointByFileAndLine);
        QVariant v = map.value(_("filename"));
        if (v.isValid())
            data.fileName = v.toString();
        v = map.value(_("linenumber"));
        if (v.isValid())
            data.lineNumber = v.toString().toInt();
        v = map.value(_("condition"));
        if (v.isValid())
            data.condition = v.toString().toLatin1();
        v = map.value(_("address"));
        if (v.isValid())
            data.address = v.toString().toULongLong();
        v = map.value(_("ignorecount"));
        if (v.isValid())
            data.ignoreCount = v.toString().toInt();
        v = map.value(_("threadspec"));
        if (v.isValid())
            data.threadSpec = v.toString().toInt();
        v = map.value(_("funcname"));
        if (v.isValid())
            data.functionName = v.toString();
        v = map.value(_("disabled"));
        if (v.isValid())
            data.enabled = !v.toInt();
        v = map.value(_("usefullpath"));
        if (v.isValid())
            data.pathUsage = static_cast<BreakpointPathUsage>(v.toInt());
        v = map.value(_("tracepoint"));
        if (v.isValid())
            data.tracepoint = bool(v.toInt());
        v = map.value(_("type"));
        if (v.isValid() && v.toInt() != UnknownType)
            data.type = BreakpointType(v.toInt());
        v = map.value(_("module"));
        if (v.isValid())
            data.module = v.toString();
        v = map.value(_("command"));
        if (v.isValid())
            data.command = v.toString();
        appendBreakpoint(data);
    }
    //qDebug() << "LOADED BREAKPOINTS" << this << list.size();
}

void BreakHandler::updateMarkers()
{
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it)
        updateMarker(it.key());
}

void BreakHandler::updateMarker(BreakpointId id)
{
    Iterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return);

    QString markerFileName = it->markerFileName();
    int markerLineNumber = it->markerLineNumber();
    if (it->marker && (markerFileName != it->marker->fileName()
                || markerLineNumber != it->marker->lineNumber()))
        it->destroyMarker();

    if (!it->marker && !markerFileName.isEmpty() && markerLineNumber > 0)
        it->marker = new BreakpointMarker(id, markerFileName, markerLineNumber);
}

QVariant BreakHandler::headerData(int section,
    Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static QString headers[] = {
            tr("Number"),  tr("Function"), tr("File"), tr("Line"),
            tr("Address"), tr("Condition"), tr("Ignore"), tr("Threads")
        };
        return headers[section];
    }
    return QVariant();
}

BreakpointId BreakHandler::findBreakpointByIndex(const QModelIndex &index) const
{
    int r = index.row();
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for (int i = 0; it != et; ++it, ++i)
        if (i == r)
            return it.key();
    return BreakpointId();
}

BreakpointIds BreakHandler::findBreakpointsByIndex(const QList<QModelIndex> &list) const
{
    QSet<BreakpointId> ids;
    foreach (const QModelIndex &index, list)
        ids.insert(findBreakpointByIndex(index));
    return ids.toList();
}

Qt::ItemFlags BreakHandler::flags(const QModelIndex &index) const
{
//    switch (index.column()) {
//        //case 0:
//        //    return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
//        default:
            return QAbstractTableModel::flags(index);
//    }
}

QString BreakHandler::displayFromThreadSpec(int spec)
{
    return spec == -1 ? BreakHandler::tr("(all)") : QString::number(spec);
}

int BreakHandler::threadSpecFromDisplay(const QString &str)
{
    bool ok = false;
    int result = str.toInt(&ok);
    return ok ? result : -1;
}

QVariant BreakHandler::data(const QModelIndex &mi, int role) const
{
    static const QString empty = QString(QLatin1Char('-'));

    if (!mi.isValid())
        return QVariant();

    BreakpointId id = findBreakpointByIndex(mi);
    //qDebug() << "DATA: " << id << role << mi.column();
    ConstIterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return QVariant());
    const BreakpointParameters &data = it->data;
    const BreakpointResponse &response = it->response;

    bool orig = false;
    switch (it->state) {
        case BreakpointInsertRequested:
        case BreakpointInsertProceeding:
        case BreakpointChangeRequested:
        case BreakpointChangeProceeding:
        case BreakpointInserted:
        case BreakpointRemoveRequested:
        case BreakpointRemoveProceeding:
            break;
        case BreakpointNew:
        case BreakpointDead:
            orig = true;
            break;
    };

    switch (mi.column()) {
        case 0:
            if (role == Qt::DisplayRole) {
                return QString::number(id);
                //return QString("%1 - %2").arg(id).arg(response.number);
            }
            if (role == Qt::DecorationRole)
                return it->icon();
            break;
        case 1:
            if (role == Qt::DisplayRole) {
                if (!response.functionName.isEmpty())
                    return response.functionName;
                if (!data.functionName.isEmpty())
                    return data.functionName;
                if (data.type == BreakpointAtMain
                        || data.type == BreakpointAtThrow
                        || data.type == BreakpointAtCatch
                        || data.type == BreakpointAtFork
                        || data.type == BreakpointAtExec
                        || data.type == BreakpointAtVFork
                        || data.type == BreakpointAtSysCall)
                    return typeToString(data.type);
                if (data.type == Watchpoint)
                    return tr("Watchpoint at 0x%1").arg(data.address, 0, 16);
                return empty;
            }
            break;
        case 2:
            if (role == Qt::DisplayRole) {
                QString str;
                if (!response.fileName.isEmpty())
                    str = response.fileName;
                if (str.isEmpty() && !data.fileName.isEmpty())
                    str = data.fileName;
                if (str.isEmpty()) {
                    QString s = QFileInfo(str).fileName();
                    if (!s.isEmpty())
                        str = s;
                }
                // FIXME: better?
                //if (data.multiple && str.isEmpty() && !response.fileName.isEmpty())
                //    str = response.fileName;
                if (!str.isEmpty())
                    return QDir::toNativeSeparators(str);
                return empty;
            }
            break;
        case 3:
            if (role == Qt::DisplayRole) {
                if (response.lineNumber > 0)
                    return response.lineNumber;
                if (data.lineNumber > 0)
                    return data.lineNumber;
                return empty;
            }
            if (role == Qt::UserRole + 1)
                return data.lineNumber;
            break;
        case 4:
            if (role == Qt::DisplayRole) {
                QString displayValue;
                const quint64 address = orig ? data.address : response.address;
                if (address)
                    displayValue += QString::fromAscii("0x%1").arg(address, 0, 16);
                if (0 && !response.extra.isEmpty()) {
                    if (!displayValue.isEmpty())
                        displayValue += QLatin1Char(' ');
                    displayValue += QString::fromAscii(response.extra);
                }
                return displayValue;
            }
            break;
        case 5:
            if (role == Qt::DisplayRole)
                return orig ? data.condition : response.condition;
            if (role == Qt::ToolTipRole)
                return tr("Breakpoint will only be hit if this condition is met.");
            if (role == Qt::UserRole + 1)
                return data.condition;
            break;
        case 6:
            if (role == Qt::DisplayRole) {
                const int ignoreCount =
                    orig ? data.ignoreCount : response.ignoreCount;
                return ignoreCount ? QVariant(ignoreCount) : QVariant(QString());
            }
            if (role == Qt::ToolTipRole)
                return tr("Breakpoint will only be hit after being ignored so many times.");
            if (role == Qt::UserRole + 1)
                return data.ignoreCount;
            break;
        case 7:
            if (role == Qt::DisplayRole)
                return displayFromThreadSpec(orig ? data.threadSpec : response.threadSpec);
            if (role == Qt::ToolTipRole)
                return tr("Breakpoint will only be hit in the specified thread(s).");
            if (role == Qt::UserRole + 1)
                return displayFromThreadSpec(data.threadSpec);
            break;
    }
    switch (role) {
    case Qt::ToolTipRole:
        if (debuggerCore()->boolSetting(UseToolTipsInBreakpointsView))
                return QVariant(it->toToolTip());
        break;
    case EngineCapabilitiesRole:  {
        const unsigned caps = it.value().engine ?
                              it.value().engine->debuggerCapabilities() :
                              unsigned(AllDebuggerCapabilities);
        return QVariant(caps);
    }
    }
    return QVariant();
}

#define GETTER(type, getter) \
type BreakHandler::getter(BreakpointId id) const \
{ \
    ConstIterator it = m_storage.find(id); \
    QTC_ASSERT(it != m_storage.end(), \
        qDebug() << "ID" << id << "NOT KNOWN"; \
        return type()); \
    return it->data.getter; \
}

#define SETTER(type, getter, setter) \
void BreakHandler::setter(BreakpointId id, const type &value) \
{ \
    Iterator it = m_storage.find(id); \
    QTC_ASSERT(it != m_storage.end(), \
        qDebug() << "ID" << id << "NOT KNOWN"; return); \
    if (it->data.getter == value) \
        return; \
    it->data.getter = value; \
    if (it->state != BreakpointNew) { \
        it->state = BreakpointChangeRequested; \
        scheduleSynchronization(); \
    } \
}

#define PROPERTY(type, getter, setter) \
    GETTER(type, getter) \
    SETTER(type, getter, setter)


PROPERTY(BreakpointPathUsage, pathUsage, setPathUsage)
PROPERTY(QString, fileName, setFileName)
PROPERTY(QString, functionName, setFunctionName)
PROPERTY(BreakpointType, type, setType)
PROPERTY(int, threadSpec, setThreadSpec)
PROPERTY(QByteArray, condition, setCondition)
GETTER(int, lineNumber)
PROPERTY(quint64, address, setAddress)
PROPERTY(int, ignoreCount, setIgnoreCount)

bool BreakHandler::isEnabled(BreakpointId id) const
{
    ConstIterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return false);
    return it->data.enabled;
}

void BreakHandler::setEnabled(BreakpointId id, bool on)
{
    Iterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return);
    //qDebug() << "SET ENABLED: " << id << it->data.isEnabled() << on;
    if (it->data.enabled == on)
        return;
    it->data.enabled = on;
    it->destroyMarker();
    updateMarker(id);
    if (it->engine) {
        it->state = BreakpointChangeRequested;
        scheduleSynchronization();
    }
}

bool BreakHandler::isTracepoint(BreakpointId id) const
{
    ConstIterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return false);
    return it->data.tracepoint;
}

void BreakHandler::setTracepoint(BreakpointId id, bool on)
{
    Iterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return);
    if (it->data.tracepoint == on)
        return;
    it->data.tracepoint = on;
    it->destroyMarker();

    updateMarker(id);
    if (it->engine) {
        it->state = BreakpointChangeRequested;
        scheduleSynchronization();
    }
}

void BreakHandler::setMarkerFileAndLine(BreakpointId id,
    const QString &fileName, int lineNumber)
{
    Iterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), qDebug() << id; return);
    if (it->response.fileName == fileName && it->response.lineNumber == lineNumber)
        return;
    it->response.fileName = fileName;
    it->response.lineNumber = lineNumber;
    it->destroyMarker();
    updateMarker(id);
    emit layoutChanged();
}

BreakpointState BreakHandler::state(BreakpointId id) const
{
    ConstIterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), qDebug() << id; return BreakpointDead);
    return it->state;
}

DebuggerEngine *BreakHandler::engine(BreakpointId id) const
{
    ConstIterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), qDebug() << id; return 0);
    return it->engine;
}

void BreakHandler::setEngine(BreakpointId id, DebuggerEngine *value)
{
    Iterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), qDebug() << id; return);
    QTC_ASSERT(it->state == BreakpointNew, qDebug() << id);
    QTC_ASSERT(!it->engine, qDebug() << id; return);
    it->engine = value;
    it->state = BreakpointInsertRequested;
    it->response = BreakpointResponse();
    it->response.fileName = it->data.fileName;
    updateMarker(id);
    scheduleSynchronization();
}

static bool isAllowedTransition(BreakpointState from, BreakpointState to)
{
    switch (from) {
    case BreakpointNew:
        return to == BreakpointInsertRequested;
    case BreakpointInsertRequested:
        return to == BreakpointInsertProceeding;
    case BreakpointInsertProceeding:
        return to == BreakpointInserted
            || to == BreakpointDead
            || to == BreakpointChangeRequested;
    case BreakpointChangeRequested:
        return to == BreakpointChangeProceeding;
    case BreakpointChangeProceeding:
        return to == BreakpointInserted
            || to == BreakpointDead;
    case BreakpointInserted:
        return to == BreakpointChangeRequested
            || to == BreakpointRemoveRequested;
    case BreakpointRemoveRequested:
        return to == BreakpointRemoveProceeding;
    case BreakpointRemoveProceeding:
        return to == BreakpointDead;
    case BreakpointDead:
        return false;
    }
    qDebug() << "UNKNOWN BREAKPOINT STATE:" << from;
    return false;
}

void BreakHandler::setState(BreakpointId id, BreakpointState state)
{
    Iterator it = m_storage.find(id);
    //qDebug() << "BREAKPOINT STATE TRANSITION" << id << it->state << state;
    QTC_ASSERT(it != m_storage.end(), qDebug() << id; return);
    QTC_ASSERT(isAllowedTransition(it->state, state),
        qDebug() << "UNEXPECTED BREAKPOINT STATE TRANSITION"
            << it->state << state);

    if (it->state == state) {
        qDebug() << "STATE UNCHANGED: " << id << state;
        return;
    }

    it->state = state;
    layoutChanged();
}

void BreakHandler::notifyBreakpointChangeAfterInsertNeeded(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointInsertProceeding, qDebug() << state(id));
    setState(id, BreakpointChangeRequested);
}

void BreakHandler::notifyBreakpointInsertProceeding(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointInsertRequested, qDebug() << state(id));
    setState(id, BreakpointInsertProceeding);
}

void BreakHandler::notifyBreakpointInsertOk(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointInsertProceeding, qDebug() << state(id));
    setState(id, BreakpointInserted);
    ConstIterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return);
}

void BreakHandler::notifyBreakpointInsertFailed(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointInsertProceeding, qDebug() << state(id));
    setState(id, BreakpointDead);
}

void BreakHandler::notifyBreakpointRemoveProceeding(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointRemoveRequested, qDebug() << state(id));
    setState(id, BreakpointRemoveProceeding);
}

void BreakHandler::notifyBreakpointRemoveOk(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointRemoveProceeding, qDebug() << state(id));
    setState(id, BreakpointDead);
    cleanupBreakpoint(id);
}

void BreakHandler::notifyBreakpointRemoveFailed(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointRemoveProceeding, qDebug() << state(id));
    setState(id, BreakpointDead);
    cleanupBreakpoint(id);
}

void BreakHandler::notifyBreakpointChangeProceeding(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointChangeRequested, qDebug() << state(id));
    setState(id, BreakpointChangeProceeding);
}

void BreakHandler::notifyBreakpointChangeOk(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointChangeProceeding, qDebug() << state(id));
    setState(id, BreakpointInserted);
}

void BreakHandler::notifyBreakpointChangeFailed(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointChangeProceeding, qDebug() << state(id));
    setState(id, BreakpointDead);
}

void BreakHandler::notifyBreakpointReleased(BreakpointId id)
{
    //QTC_ASSERT(state(id) == BreakpointChangeProceeding, qDebug() << state(id));
    Iterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return);
    it->state = BreakpointNew;
    it->engine = 0;
    it->response = BreakpointResponse();
    delete it->marker;
    it->marker = 0;
    updateMarker(id);
    layoutChanged();
}

void BreakHandler::notifyBreakpointAdjusted(BreakpointId id,
        const BreakpointParameters &data)
{
    QTC_ASSERT(state(id) == BreakpointInserted, qDebug() << state(id));
    Iterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return);
    it->data = data;
    //if (it->needsChange())
    //    setState(id, BreakpointChangeRequested);
}

void BreakHandler::notifyBreakpointNeedsReinsertion(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointChangeProceeding, qDebug() << state(id));
    Iterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return);
    it->state = BreakpointInsertRequested;
}

void BreakHandler::removeBreakpoint(BreakpointId id)
{
    Iterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return);
    switch (it->state) {
    case BreakpointInserted:
        setState(id, BreakpointRemoveRequested);
        scheduleSynchronization();
        break;
    case BreakpointNew:
        it->state = BreakpointDead;
        cleanupBreakpoint(id);
        break;
    default:
        qWarning("Warning: Cannot remove breakpoint %llu in state '%s'.",
               id, qPrintable(stateToString(it->state)));
        it->state = BreakpointRemoveRequested;
        break;
    }
}

void BreakHandler::appendBreakpoint(const BreakpointParameters &data)
{
    QTC_ASSERT(data.type != UnknownType, return);

    // Ok to be not thread-safe. The order does not matter and only the gui
    // produces authoritative ids.
    static quint64 currentId = 0;

    BreakpointId id(++currentId);
    BreakpointItem item;
    item.data = data;
    item.response.fileName = data.fileName;
    item.response.lineNumber = data.lineNumber;

    const int row = m_storage.size();
    beginInsertRows(QModelIndex(), row, row);
    m_storage.insert(id, item);
    endInsertRows();

    updateMarker(id);
    scheduleSynchronization();

}

void BreakHandler::saveSessionData()
{
    saveBreakpoints();
}

void BreakHandler::loadSessionData()
{
    m_storage.clear();
    loadBreakpoints();
}

void BreakHandler::removeSessionData()
{
    Iterator it = m_storage.begin(), et = m_storage.end();
    for ( ; it != et; ++it)
        it->destroyMarker();
    m_storage.clear();
    layoutChanged();
}

void BreakHandler::breakByFunction(const QString &functionName)
{
    // One breakpoint per function is enough for now. This does not handle
    // combinations of multiple conditions and ignore counts, though.
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it) {
        const BreakpointParameters &data = it->data;
        if (data.functionName == functionName
                && data.condition.isEmpty()
                && data.ignoreCount == 0)
            return;
    }
    BreakpointParameters data(BreakpointByFunction);
    data.functionName = functionName;
    appendBreakpoint(data);
}

QIcon BreakHandler::icon(BreakpointId id) const
{
    ConstIterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(),
        qDebug() << "NO ICON FOR ID" << id;
        return pendingBreakpointIcon());
    return it->icon();
}

void BreakHandler::scheduleSynchronization()
{
    if (m_syncTimerId == -1)
        m_syncTimerId = startTimer(10);
}

void BreakHandler::timerEvent(QTimerEvent *event)
{
    QTC_ASSERT(event->timerId() == m_syncTimerId, return);
    killTimer(m_syncTimerId);
    m_syncTimerId = -1;
    saveBreakpoints();  // FIXME: remove?
    debuggerCore()->synchronizeBreakpoints();
}

void BreakHandler::gotoLocation(BreakpointId id) const
{
    ConstIterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return);
    DebuggerEngine *engine = debuggerCore()->currentEngine();
    if (it->data.type == BreakpointByAddress) {
        if (engine)
            engine->gotoLocation(it->data.address);
    } else {
        if (engine)
            engine->gotoLocation(
                Location(it->markerFileName(), it->markerLineNumber(), false));
    }
}

void BreakHandler::updateLineNumberFromMarker(BreakpointId id, int lineNumber)
{
    Iterator it = m_storage.find(id);
    it->response.pending = false;
    QTC_ASSERT(it != m_storage.end(), return);
    if (it->response.lineNumber != lineNumber) {
        // FIXME: Should we tell gdb about the change?
        it->response.lineNumber = lineNumber;
    }
    // Ignore updates to the "real" line number while the debugger is
    // running, as this can be triggered by moving the breakpoint to
    // the next line that generated code.
    if (it->response.number == 0) {
        // FIXME: Do we need yet another data member?
        it->data.lineNumber = lineNumber;
    }
    updateMarker(id);
    emit layoutChanged();
}

BreakpointIds BreakHandler::allBreakpointIds() const
{
    BreakpointIds ids;
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it)
        ids.append(it.key());
    return ids;
}

BreakpointIds BreakHandler::unclaimedBreakpointIds() const
{
    return engineBreakpointIds(0);
}

BreakpointIds BreakHandler::engineBreakpointIds(DebuggerEngine *engine) const
{
    BreakpointIds ids;
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it)
        if (it->engine == engine)
            ids.append(it.key());
    return ids;
}

void BreakHandler::cleanupBreakpoint(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointDead, qDebug() << state(id));
    BreakpointItem item = m_storage.take(id);
    item.destroyMarker();
    layoutChanged();
}

const BreakpointResponse &BreakHandler::response(BreakpointId id) const
{
    static BreakpointResponse dummy;
    ConstIterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), qDebug() << id; return dummy);
    return it->response;
}

bool BreakHandler::needsChange(BreakpointId id) const
{
    ConstIterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return false);
    return it->needsChange();
}

void BreakHandler::setResponse(BreakpointId id,
    const BreakpointResponse &response, bool takeOver)
{
    Iterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return);
    BreakpointItem &item = it.value();
    item.response = response;
    item.destroyMarker();
    // Take over corrected values from response.
    if (takeOver) {
        if (item.data.type == BreakpointByFileAndLine
            && response.correctedLineNumber > 0)
            item.data.lineNumber = response.correctedLineNumber;
        if ((item.data.type == BreakpointByFileAndLine
                    || item.data.type == BreakpointByFunction)
                && !response.module.isEmpty())
            item.data.module = response.module;
    }
    updateMarker(id);
}

void BreakHandler::setBreakpointData(BreakpointId id,
    const BreakpointParameters &data)
{
    Iterator it = m_storage.find(id);
    QTC_ASSERT(it != m_storage.end(), return);
    if (data == it->data)
        return;
    it->data = data;
    if (it->needsChange() && it->engine && it->state != BreakpointNew) {
        setState(id, BreakpointChangeRequested);
        scheduleSynchronization();
    } else {
        it->destroyMarker();
        updateMarker(id);
        layoutChanged();
    }
}

//////////////////////////////////////////////////////////////////
//
// Storage
//
//////////////////////////////////////////////////////////////////

BreakHandler::BreakpointItem::BreakpointItem()
  : state(BreakpointNew), engine(0), marker(0)
{}

void BreakHandler::BreakpointItem::destroyMarker()
{
    BreakpointMarker *m = marker;
    marker = 0;
    delete m;
}

QString BreakHandler::BreakpointItem::markerFileName() const
{
    // Some heuristics to find a "good" file name.
    if (!data.fileName.isEmpty()) {
        QFileInfo fi(data.fileName);
        if (fi.exists())
            return fi.absoluteFilePath();
    }
    if (!response.fileName.isEmpty()) {
        QFileInfo fi(response.fileName);
        if (fi.exists())
            return fi.absoluteFilePath();
    }
    if (response.fileName.endsWith(data.fileName))
        return response.fileName;
    if (data.fileName.endsWith(response.fileName))
        return data.fileName;
    return response.fileName.size() > data.fileName.size()
        ? response.fileName : data.fileName;
}


int BreakHandler::BreakpointItem::markerLineNumber() const
{
    return response.lineNumber ? response.lineNumber : data.lineNumber;
}

static void formatAddress(QTextStream &str, quint64 address)
{
    if (address) {
        str << "0x";
        str.setIntegerBase(16);
        str << address;
        str.setIntegerBase(10);
    }
}

bool BreakHandler::BreakpointItem::needsChange() const
{
    if (!data.conditionsMatch(response.condition))
        return true;
    if (data.ignoreCount != response.ignoreCount)
        return true;
    if (data.enabled != response.enabled)
        return true;
    if (data.threadSpec != response.threadSpec)
        return true;
    if (data.command != response.command)
        return true;
    return false;
}

bool BreakHandler::BreakpointItem::isLocatedAt
    (const QString &fileName, int lineNumber, bool useMarkerPosition) const
{
    int line = useMarkerPosition ? response.lineNumber : data.lineNumber;
    return lineNumber == line
        && (fileNameMatch(fileName, response.fileName)
            || fileNameMatch(fileName, markerFileName()));
}

QIcon BreakHandler::BreakpointItem::icon() const
{
    // FIXME: This seems to be called on each cursor blink as soon as the
    // cursor is near a line with a breakpoint marker (+/- 2 lines or so).
    if (data.isTracepoint())
        return BreakHandler::tracepointIcon();
    if (data.type == Watchpoint)
        return BreakHandler::watchpointIcon();
    if (!data.enabled)
        return BreakHandler::disabledBreakpointIcon();
    if (state == BreakpointInserted)
        return BreakHandler::breakpointIcon();
    return BreakHandler::pendingBreakpointIcon();
}

QString BreakHandler::BreakpointItem::toToolTip() const
{
    QString rc;
    QTextStream str(&rc);
    str << "<html><body><table>"
        //<< "<tr><td>" << tr("Id:") << "</td><td>" << m_id << "</td></tr>"
        << "<tr><td>" << tr("State:")
        << "</td><td>" << (data.enabled ? tr("Enabled") : tr("Disabled"));
    if (response.pending)
        str << tr(", pending");
    str << ", " << state << "   (" << stateToString(state) << ")</td></tr>";
    if (engine) {
        str << "<tr><td>" << tr("Engine:")
        << "</td><td>" << engine->objectName() << "</td></tr>";
    }
    if (!response.pending) {
        str << "<tr><td>" << tr("Breakpoint Number:")
            << "</td><td>" << response.number << "</td></tr>";
    }
    str << "<tr><td>" << tr("Breakpoint Type:")
        << "</td><td>" << typeToString(data.type) << "</td></tr>";
    if (!response.extra.isEmpty()) {
        str << "<tr><td>" << tr("Extra Information:")
            << "</td><td>" << response.extra << "</td></tr>";    }
    str << "<tr><td>" << tr("Marker File:")
        << "</td><td>" << QDir::toNativeSeparators(markerFileName()) << "</td></tr>"
        << "<tr><td>" << tr("Marker Line:")
        << "</td><td>" << markerLineNumber() << "</td></tr>"
        << "</table><br><hr><table>"
        << "<tr><th>" << tr("Property")
        << "</th><th>" << tr("Requested")
        << "</th><th>" << tr("Obtained") << "</th></tr>"
        << "<tr><td>" << tr("Internal Number:")
        << "</td><td>&mdash;</td><td>" << response.number << "</td></tr>";
    if (data.type == BreakpointByFunction) {
        str << "<tr><td>" << tr("Function Name:")
        << "</td><td>" << data.functionName
        << "</td><td>" << response.functionName
        << "</td></tr>";
    }
    if (data.type == BreakpointByFileAndLine) {
    str << "<tr><td>" << tr("File Name:")
        << "</td><td>" << QDir::toNativeSeparators(data.fileName)
        << "</td><td>" << QDir::toNativeSeparators(response.fileName)
        << "</td></tr>"
        << "<tr><td>" << tr("Line Number:")
        << "</td><td>" << data.lineNumber
        << "</td><td>" << response.lineNumber << "</td></tr>"
        << "<tr><td>" << tr("Corrected Line Number:")
        << "</td><td>-"
        << "</td><td>" << response.correctedLineNumber << "</td></tr>";
    }
    if (data.type == BreakpointByFunction || data.type == BreakpointByFileAndLine) {
        str << "<tr><td>" << tr("Module:")
            << "</td><td>" << data.module
            << "</td><td>" << response.module
            << "</td></tr>";
    }
    str << "<tr><td>" << tr("Breakpoint Address:")
        << "</td><td>";
    formatAddress(str, data.address);
    str << "</td><td>";
    formatAddress(str, response.address);
    str << "</td></tr>";
    if (response.multiple) {
        str << "<tr><td>" << tr("Multiple Addresses:")
            << "</td><td>";
        foreach (quint64 address, response.addresses) {
            formatAddress(str, address);
            str << " ";
        }
        str << "</td></tr>";
    }
    if (!data.command.isEmpty() || !response.command.isEmpty()) {
        str << "<tr><td>" << tr("Command:")
            << "</td><td>" << data.command
            << "</td><td>" << response.command
            << "</td></tr>";
    }
    if (!data.condition.isEmpty() || !response.condition.isEmpty()) {
        str << "<tr><td>" << tr("Condition:")
            << "</td><td>" << data.condition
            << "</td><td>" << response.condition
            << "</td></tr>";
    }
    if (data.ignoreCount || response.ignoreCount) {
        str << "<tr><td>" << tr("Ignore Count:") << "</td><td>";
        if (data.ignoreCount)
            str << data.ignoreCount;
        str << "</td><td>";
        if (response.ignoreCount)
            str << response.ignoreCount;
        str << "</td></tr>";
    }
    if (data.threadSpec >= 0 || response.threadSpec >= 0) {
        str << "<tr><td>" << tr("Thread Specification:")
            << "</td><td>";
        if (data.threadSpec >= 0)
            str << data.threadSpec;
        str << "</td><td>";
        if (response.threadSpec >= 0)
            str << response.threadSpec;
        str << "</td></tr>";
    }
    str  << "</table></body></html>";
    return rc;
}

} // namespace Internal
} // namespace Debugger
