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

#include "breakhandler.h"
#include "breakpointmarker.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"

#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>


//////////////////////////////////////////////////////////////////
//
// BreakHandler
//
//////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

BreakHandler::BreakHandler()
  : m_breakpointIcon(_(":/debugger/images/breakpoint_16.png")),
    m_disabledBreakpointIcon(_(":/debugger/images/breakpoint_disabled_16.png")),
    m_pendingBreakPointIcon(_(":/debugger/images/breakpoint_pending_16.png")),
    //m_emptyIcon(_(":/debugger/images/watchpoint.png")),
    m_emptyIcon(_(":/debugger/images/breakpoint_pending_16.png")),
    //m_emptyIcon(_(":/debugger/images/debugger_empty_14.png")),
    m_watchpointIcon(_(":/debugger/images/watchpoint.png")),
    m_syncTimerId(-1)
{}

BreakHandler::~BreakHandler()
{}

int BreakHandler::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 8;
}

int BreakHandler::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_bp.size();
}

// FIXME: Only used by cdb. Move there?
bool BreakHandler::hasPendingBreakpoints() const
{
    ConstIterator it = m_bp.constBegin(), et = m_bp.constEnd();
    for ( ; it != et; ++it)
        if (it.value()->isPending())
            return true;
    return false;
}

static inline bool fileNameMatch(const QString &f1, const QString &f2)
{
#ifdef Q_OS_WIN
    return f1.compare(f2, Qt::CaseInsensitive) == 0;
#else
    return f1 == f2;
#endif
}

static bool isSimilarTo(const BreakpointData *data, const BreakpointResponse &needle)
{
    // Clear hit.
    // Clear miss.
    if (needle.bpType != UnknownType && data->type() != UnknownType
            && data->type() != needle.bpType)
        return false;

    // Clear hit.
    if (data->address() && data->address() == needle.bpAddress)
        return true;

    // At least at a position we were looking for.
    // FIXME: breaks multiple breakpoints at the same location
    if (!data->fileName().isEmpty()
            && fileNameMatch(data->fileName(), needle.bpFileName)
            && data->lineNumber() == needle.bpLineNumber)
        return true;

    // At least at a position we were looking for.
    // FIXME: breaks multiple breakpoints at the same location
    if (!data->fileName().isEmpty()
            && fileNameMatch(data->fileName(), needle.bpFileName)
            && data->lineNumber() == needle.bpLineNumber)
        return true;

    return false;
}

BreakpointId BreakHandler::findSimilarBreakpoint(const BreakpointResponse &needle) const
{
    // Search a breakpoint we might refer to.
    ConstIterator it = m_bp.constBegin(), et = m_bp.constEnd();
    for ( ; it != et; ++it) {
        const BreakpointId id = it.key();
        const BreakpointData *data = it.value();
        QTC_ASSERT(data, continue);
        const BreakpointResponse *response = m_responses.value(id);
        QTC_ASSERT(response, continue);

        qDebug() << "COMPARING " << data->toString() << " WITH " << needle.toString();
        if (response->bpNumber && response->bpNumber == needle.bpNumber)
            return id;

        if (isSimilarTo(data, needle))
            return id;
    }
    return BreakpointId(-1);
}

BreakpointId BreakHandler::findBreakpointByNumber(int bpNumber) const
{
    Responses::ConstIterator it = m_responses.constBegin();
    Responses::ConstIterator et = m_responses.constEnd();
    for ( ; it != et; ++it)
        if (it.value()->bpNumber == bpNumber)
            return it.key();
    return BreakpointId(-1);
}

BreakpointId BreakHandler::findBreakpointByFunction(const QString &functionName) const
{
    ConstIterator it = m_bp.constBegin(), et = m_bp.constEnd();
    for ( ; it != et; ++it)
        if (it.value()->functionName() == functionName)
            return it.key();
    return BreakpointId(-1);
}

BreakpointId BreakHandler::findBreakpointByAddress(quint64 address) const
{
    ConstIterator it = m_bp.constBegin(), et = m_bp.constEnd();
    for ( ; it != et; ++it)
        if (it.value()->address() == address)
            return it.key();
    return BreakpointId(-1);
}

BreakpointId BreakHandler::findBreakpointByFileAndLine(const QString &fileName,
    int lineNumber, bool useMarkerPosition)
{
    ConstIterator it = m_bp.constBegin(), et = m_bp.constEnd();
    for ( ; it != et; ++it)
        if (it.value()->isLocatedAt(fileName, lineNumber, useMarkerPosition))
            return it.key();
    return BreakpointId(-1);
}

BreakpointData *BreakHandler::breakpointById(BreakpointId id) const
{
    return m_bp.value(id, 0);
}

BreakpointId BreakHandler::findWatchpointByAddress(quint64 address) const
{
    ConstIterator it = m_bp.constBegin(), et = m_bp.constEnd();
    for ( ; it != et; ++it)
        if (it.value()->isWatchpoint() && it.value()->address() == address)
            return it.key();
    return BreakpointId(-1);
}


void BreakHandler::setWatchpointByAddress(quint64 address)
{
    const int id = findWatchpointByAddress(address);
    if (id == -1) {
        BreakpointData *data = new BreakpointData;
        data->setType(Watchpoint);
        data->setAddress(address);
        appendBreakpoint(data);
        scheduleSynchronization();
    } else {
        qDebug() << "WATCHPOINT EXISTS";
     //   removeBreakpoint(index);
    }
}

bool BreakHandler::hasWatchpointAt(quint64 address) const
{
    return findWatchpointByAddress(address) != BreakpointId(-1);
}

void BreakHandler::saveBreakpoints()
{
    //qDebug() << "SAVING BREAKPOINTS...";
    QTC_ASSERT(debuggerCore(), return);
    QList<QVariant> list;
    ConstIterator it = m_bp.constBegin(), et = m_bp.constEnd();
    for ( ; it != et; ++it) {
        const BreakpointData *data = it.value();
        QMap<QString, QVariant> map;
        // Do not persist Watchpoints.
        //if (data->isWatchpoint())
        //    continue;
        if (data->type() != BreakpointByFileAndLine)
            map.insert(_("type"), data->type());
        if (!data->fileName().isEmpty())
            map.insert(_("filename"), data->fileName());
        if (data->lineNumber())
            map.insert(_("linenumber"), data->lineNumber());
        if (!data->functionName().isEmpty())
            map.insert(_("funcname"), data->functionName());
        if (data->address())
            map.insert(_("address"), data->address());
        if (!data->condition().isEmpty())
            map.insert(_("condition"), data->condition());
        if (data->ignoreCount())
            map.insert(_("ignorecount"), data->ignoreCount());
        if (!data->threadSpec().isEmpty())
            map.insert(_("threadspec"), data->threadSpec());
        if (!data->isEnabled())
            map.insert(_("disabled"), _("1"));
        if (data->useFullPath())
            map.insert(_("usefullpath"), _("1"));
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
        BreakpointData *data = new BreakpointData;
        QVariant v = map.value(_("filename"));
        if (v.isValid())
            data->setFileName(v.toString());
        v = map.value(_("linenumber"));
        if (v.isValid())
            data->setLineNumber(v.toString().toInt());
        v = map.value(_("condition"));
        if (v.isValid())
            data->setCondition(v.toString().toLatin1());
        v = map.value(_("address"));
        if (v.isValid())
            data->setAddress(v.toString().toULongLong());
        v = map.value(_("ignorecount"));
        if (v.isValid())
            data->setIgnoreCount(v.toString().toInt());
        v = map.value(_("threadspec"));
        if (v.isValid())
            data->setThreadSpec(v.toString().toLatin1());
        v = map.value(_("funcname"));
        if (v.isValid())
            data->setFunctionName(v.toString());
        v = map.value(_("disabled"));
        if (v.isValid())
            data->setEnabled(!v.toInt());
        v = map.value(_("usefullpath"));
        if (v.isValid())
            data->setUseFullPath(bool(v.toInt()));
        v = map.value(_("type"));
        if (v.isValid())
            data->setType(BreakpointType(v.toInt()));
        data->setMarkerFileName(data->fileName());
        data->setMarkerLineNumber(data->lineNumber());
        appendBreakpoint(data);
    }
    //qDebug() << "LOADED BREAKPOINTS" << this << list.size();
}

void BreakHandler::updateMarkers()
{
    ConstIterator it = m_bp.constBegin(), et = m_bp.constEnd();
    for ( ; it != et; ++it)
        updateMarker(it.key());
}

void BreakHandler::updateMarker(BreakpointId id)
{
    BreakpointData *data = m_bp.value(id);
    QTC_ASSERT(data, return);

    BreakpointMarker *marker = m_markers.value(id);

    if (marker && (data->m_markerFileName != marker->fileName()
                || data->m_markerLineNumber != marker->lineNumber())) {
        removeMarker(id);
        marker = 0;
    }

    if (!marker && !data->m_markerFileName.isEmpty() && data->m_markerLineNumber > 0) {
        marker = new BreakpointMarker(id, data->m_markerFileName, data->m_markerLineNumber);
        m_markers.insert(id, marker);
    }
}

void BreakHandler::removeMarker(BreakpointId id)
{
    delete m_markers.take(id);
}

QVariant BreakHandler::headerData(int section,
    Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static QString headers[] = {
            tr("Number"),  tr("Function"), tr("File"), tr("Line"),
            tr("Condition"), tr("Ignore"), tr("Threads"), tr("Address")
        };
        return headers[section];
    }
    return QVariant();
}

BreakpointId BreakHandler::findBreakpointByIndex(const QModelIndex &index) const
{
    int r = index.row();
    ConstIterator it = m_bp.constBegin(), et = m_bp.constEnd();
    for (int i = 0; it != et; ++it, ++i)
        if (i == r)
            return it.key();
    return BreakpointId(-1);
}

QVariant BreakHandler::data(const QModelIndex &mi, int role) const
{
    static const QString empty = QString(QLatin1Char('-'));

    QTC_ASSERT(mi.isValid(), return QVariant());
    BreakpointId id = findBreakpointByIndex(mi);
    BreakpointData *data = m_bp.value(id);
    QTC_ASSERT(data, return QVariant());
    BreakpointResponse *response = m_responses.value(id, 0);
    QTC_ASSERT(response, return QVariant());

    switch (mi.column()) {
        case 0:
            if (role == Qt::DisplayRole) {
                return QString("%1 - %2").arg(id).arg(response->bpNumber);
            }
            if (role == Qt::DecorationRole) {
                if (data->isWatchpoint())
                    return m_watchpointIcon;
                if (!data->isEnabled())
                    return m_disabledBreakpointIcon;
                return data->isPending() ? m_pendingBreakPointIcon : m_breakpointIcon;
            }
            break;
        case 1:
            if (role == Qt::DisplayRole) {
                const QString str = data->isPending()
                    ? data->functionName() : response->bpFuncName;
                return str.isEmpty() ? empty : str;
            }
            break;
        case 2:
            if (role == Qt::DisplayRole) {
                QString str = data->isPending()
                    ? data->fileName() : response->bpFileName;
                str = QFileInfo(str).fileName();
                // FIXME: better?
                //if (data->bpMultiple && str.isEmpty() && !data->markerFileName.isEmpty())
                //    str = data->markerFileName;
                str = str.isEmpty() ? empty : str;
                if (data->useFullPath())
                    str = QDir::toNativeSeparators(QLatin1String("/.../") + str);
                return str;
            }
            break;
        case 3:
            if (role == Qt::DisplayRole) {
                // FIXME: better?
                //if (data->bpMultiple && str.isEmpty() && !data->markerFileName.isEmpty())
                //    str = data->markerLineNumber;
                const int nr = data->isPending()
                    ? data->lineNumber() : response->bpLineNumber;
                return nr ? QString::number(nr) : empty;
            }
            if (role == Qt::UserRole + 1)
                return data->lineNumber();
            break;
        case 4:
            if (role == Qt::DisplayRole)
                return data->isPending() ? data->condition() : response->bpCondition;
            if (role == Qt::ToolTipRole)
                return tr("Breakpoint will only be hit if this condition is met.");
            if (role == Qt::UserRole + 1)
                return data->condition();
            break;
        case 5:
            if (role == Qt::DisplayRole) {
                const int ignoreCount =
                    data->isPending() ? data->ignoreCount() : response->bpIgnoreCount;
                return ignoreCount ? QVariant(ignoreCount) : QVariant(QString());
            }
            if (role == Qt::ToolTipRole)
                return tr("Breakpoint will only be hit after being ignored so many times.");
            if (role == Qt::UserRole + 1)
                return data->ignoreCount();
            break;
        case 6:
            if (role == Qt::DisplayRole) {
                if (data->isPending())
                    return !data->threadSpec().isEmpty() ? data->threadSpec() : tr("(all)");
                else
                    return !response->bpThreadSpec.isEmpty() ? response->bpThreadSpec : tr("(all)");
            }
            if (role == Qt::ToolTipRole)
                return tr("Breakpoint will only be hit in the specified thread(s).");
            if (role == Qt::UserRole + 1)
                return data->threadSpec();
            break;
        case 7:
            if (role == Qt::DisplayRole) {
                QString displayValue;
                const quint64 address =
                    data->isWatchpoint() ? data->address() : response->bpAddress;
                if (address)
                    displayValue += QString::fromAscii("0x%1").arg(address, 0, 16);
                if (!response->bpState.isEmpty()) {
                    if (!displayValue.isEmpty())
                        displayValue += QLatin1Char(' ');
                    displayValue += QString::fromAscii(response->bpState);
                }
                return displayValue;
            }
            break;
    }
    if (role == Qt::ToolTipRole)
        return debuggerCore()->boolSetting(UseToolTipsInBreakpointsView)
                ? data->toToolTip() : QVariant();

    return QVariant();
}

#define GETTER(type, getter) \
type BreakHandler::getter(BreakpointId id) const \
{ \
    BreakpointData *data = m_bp.value(id); \
    QTC_ASSERT(data, return type()); \
    return data->getter(); \
}

#define SETTER(type, setter) \
void BreakHandler::setter(BreakpointId id, const type &value) \
{ \
    BreakpointData *data = m_bp.value(id); \
    QTC_ASSERT(data, return); \
    if (data->setter(value)) \
        scheduleSynchronization(); \
}

#define PROPERTY(type, getter, setter) \
    GETTER(type, getter) \
    SETTER(type, setter)


PROPERTY(bool, useFullPath, setUseFullPath)
PROPERTY(QString, markerFileName, setMarkerFileName)
PROPERTY(QString, fileName, setFileName)
PROPERTY(QString, functionName, setFunctionName)
PROPERTY(int, markerLineNumber, setMarkerLineNumber)
PROPERTY(bool, isEnabled, setEnabled)
PROPERTY(BreakpointType, type, setType)
PROPERTY(BreakpointState, state, setState)
PROPERTY(QByteArray, threadSpec, setThreadSpec)
PROPERTY(QByteArray, condition, setCondition)
PROPERTY(int, lineNumber, setLineNumber)
PROPERTY(quint64, address, setAddress)

DebuggerEngine *BreakHandler::engine(BreakpointId id) const
{
    BreakpointData *data = m_bp.value(id);
    QTC_ASSERT(data, return 0);
    return data->engine();
}

void BreakHandler::setEngine(BreakpointId id, DebuggerEngine *value)
{
    BreakpointData *data = m_bp.value(id);
    QTC_ASSERT(data, return);
    QTC_ASSERT(data->state() == BreakpointNew, /**/);
    QTC_ASSERT(!data->engine(), return);
    data->setEngine(value);
    data->setState(BreakpointInsertRequested);
    updateMarker(id);
    scheduleSynchronization();
}

void BreakHandler::ackCondition(BreakpointId id)
{
    BreakpointData *data = m_bp.value(id);
    BreakpointResponse *response = m_responses[id];
    QTC_ASSERT(data, return);
    QTC_ASSERT(response, return);
    response->bpCondition = data->condition();
    updateMarker(id);
}

void BreakHandler::ackIgnoreCount(BreakpointId id)
{
    BreakpointData *data = m_bp.value(id);
    BreakpointResponse *response = m_responses[id];
    QTC_ASSERT(data, return);
    QTC_ASSERT(response, return);
    response->bpIgnoreCount = data->ignoreCount();
    updateMarker(id);
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

void BreakHandler::removeBreakpoint(BreakpointId id)
{
    BreakpointData *data = m_bp.value(id);
    if (data->state() == BreakpointInserted) {
        qDebug() << "MARK AS CHANGED: " << id;
        data->m_state = BreakpointRemoveRequested;
        QTC_ASSERT(data->engine(), return);
        debuggerCore()->synchronizeBreakpoints();
    } else if (data->state() == BreakpointNew) {
        data->m_state = BreakpointDead;
        cleanupBreakpoint(id);
    } else {
        qDebug() << "CANNOT REMOVE IN STATE " << data->state();
    }
}

void BreakHandler::appendBreakpoint(BreakpointData *data)
{
    // Ok to be not thread-safe. The order does not matter and only the gui
    // produces authoritative ids.
    static quint64 currentId = 0;

    BreakpointId id(++currentId);
    m_bp.insert(id, data);
    m_responses.insert(id, new BreakpointResponse);
    scheduleSynchronization();
}

void BreakHandler::toggleBreakpoint(const QString &fileName, int lineNumber,
                                    quint64 address /* = 0 */)
{
    BreakpointId id(-1);

    if (address) {
        id = findBreakpointByAddress(address);
    } else {
        id = findBreakpointByFileAndLine(fileName, lineNumber, true);
        if (id == BreakpointId(-1))
            id = findBreakpointByFileAndLine(fileName, lineNumber, false);
    }

    if (id != BreakpointId(-1)) {
        removeBreakpoint(id);
    } else {
        BreakpointData *data = new BreakpointData;
        if (address) {
            data->setAddress(address);
        } else {
            data->setFileName(fileName);
            data->setLineNumber(lineNumber);
        }
        data->setMarkerFileName(fileName);
        data->setMarkerLineNumber(lineNumber);
        appendBreakpoint(data);
    }
    debuggerCore()->synchronizeBreakpoints();
}

void BreakHandler::saveSessionData()
{
    saveBreakpoints();
}

void BreakHandler::loadSessionData()
{
    qDeleteAll(m_responses);
    m_responses.clear();
    qDeleteAll(m_bp);
    m_bp.clear();
    qDeleteAll(m_markers);
    m_markers.clear();
    loadBreakpoints();
}

void BreakHandler::breakByFunction(const QString &functionName)
{
    // One breakpoint per function is enough for now. This does not handle
    // combinations of multiple conditions and ignore counts, though.
    ConstIterator it = m_bp.constBegin(), et = m_bp.constEnd();
    for ( ; it != et; ++it) {
        const BreakpointData *data = it.value();
        QTC_ASSERT(data, break);
        if (data->functionName() == functionName
                && data->condition().isEmpty()
                && data->ignoreCount() == 0)
            return;
    }
    BreakpointData *data = new BreakpointData;
    data->setType(BreakpointByFunction);
    data->setFunctionName(functionName);
    appendBreakpoint(data);
}

QIcon BreakHandler::icon(BreakpointId id) const
{
    //if (!m_handler->isActive())
    //    return m_handler->emptyIcon();
    const BreakpointData *data = m_bp.value(id);
    QTC_ASSERT(data, return pendingBreakPointIcon());
    //if (!isActive())
    //    return emptyIcon();
    switch (data->state()) {
    case BreakpointInserted:
        return breakpointIcon();
    default:
        return pendingBreakPointIcon();
    }
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
    //qDebug() << "BREAKPOINT SYNCRONIZATION STARTED";
    debuggerCore()->synchronizeBreakpoints();
    updateMarkers();
    emit layoutChanged();
    saveBreakpoints();  // FIXME: remove?
}

void BreakHandler::gotoLocation(BreakpointId id) const
{
    BreakpointData *data = m_bp.value(id);
    QTC_ASSERT(data, return);
    debuggerCore()->gotoLocation(
        data->fileName(), data->lineNumber(), false);
}

void BreakHandler::updateLineNumberFromMarker(BreakpointId id, int lineNumber)
{
    BreakpointData *data = m_bp.value(id);
    QTC_ASSERT(data, return);
    //if (data->markerLineNumber == lineNumber)
    //    return;
    if (data->markerLineNumber() != lineNumber) {
        data->setMarkerLineNumber(lineNumber);
        // FIXME: Should we tell gdb about the change?
        // Ignore it for now, as we would require re-compilation
        // and debugger re-start anyway.
        //if (0 && data->bpLineNumber) {
        //    if (!data->bpNumber.trimmed().isEmpty()) {
        //        data->pending = true;
        //    }
        //}
    }
    // Ignore updates to the "real" line number while the debugger is
    // running, as this can be triggered by moving the breakpoint to
    // the next line that generated code.
    // FIXME: Do we need yet another data member?
    BreakpointResponse *response = m_responses[id];
    QTC_ASSERT(response, return);
    if (response->bpNumber == 0) {
        data->setLineNumber(lineNumber);
        updateMarker(id);
    }
}

BreakpointIds BreakHandler::allBreakpointIds() const
{
    BreakpointIds ids;
    ConstIterator it = m_bp.constBegin(), et = m_bp.constEnd();
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
    ConstIterator it = m_bp.constBegin(), et = m_bp.constEnd();
    for ( ; it != et; ++it)
        if (it.value()->engine() == engine)
            ids.append(it.key());
    return ids;
}

void BreakHandler::notifyBreakpointInsertOk(BreakpointId id)
{
    QTC_ASSERT(breakHandler()->state(id)== BreakpointInsertProceeding, /**/);
    breakHandler()->setState(id, BreakpointInserted);
}

void BreakHandler::notifyBreakpointInsertFailed(BreakpointId id)
{
    QTC_ASSERT(breakHandler()->state(id)== BreakpointInsertProceeding, /**/);
    breakHandler()->setState(id, BreakpointDead);
}

void BreakHandler::notifyBreakpointRemoveOk(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointRemoveProceeding, /**/);
    setState(id, BreakpointDead);
    cleanupBreakpoint(id);
}

void BreakHandler::notifyBreakpointRemoveFailed(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointRemoveProceeding, /**/);
    setState(id, BreakpointDead);
    cleanupBreakpoint(id);
}

void BreakHandler::notifyBreakpointChangeOk(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointChangeProceeding, /**/);
    setState(id, BreakpointInserted);
}

void BreakHandler::notifyBreakpointChangeFailed(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointChangeProceeding, /**/);
    setState(id, BreakpointDead);
}

void BreakHandler::notifyBreakpointReleased(BreakpointId id)
{
    //QTC_ASSERT(state(id) == BreakpointChangeProceeding, /**/);
    setState(id, BreakpointNew);
    m_bp.value(id)->setEngine(0);
    delete m_markers.take(id);
    *m_responses[id] = BreakpointResponse();
    updateMarker(id);
}

void BreakHandler::cleanupBreakpoint(BreakpointId id)
{
    QTC_ASSERT(state(id) == BreakpointDead, /**/);
    delete m_markers.take(id);
    delete m_bp.take(id);
    delete m_responses.take(id);
}

BreakpointResponse BreakHandler::response(BreakpointId id) const
{
    BreakpointResponse *response = m_responses[id];
    return response ? *response : BreakpointResponse();
}

void BreakHandler::setResponse(BreakpointId id, const BreakpointResponse &data)
{
    delete m_responses.take(id);
    m_responses[id] = new BreakpointResponse(data);
    updateMarker(id);
}
#if 0
void BreakHandler::notifyBreakpointAdjusted(BreakpointId id)
{
    QTC_ASSERT(state(id)== BreakpointChangeProceeding, /**/);
    bp->bpNumber      = rbp.bpNumber;
    bp->bpCondition   = rbp.bpCondition;
    bp->bpIgnoreCount = rbp.bpIgnoreCount;
    bp->bpFileName    = rbp.bpFileName;
    bp->bpFullName    = rbp.bpFullName;
    bp->bpLineNumber  = rbp.bpLineNumber;
    bp->bpCorrectedLineNumber = rbp.bpCorrectedLineNumber;
    bp->bpThreadSpec  = rbp.bpThreadSpec;
    bp->bpFuncName    = rbp.bpFuncName;
    bp->bpAddress     = rbp.bpAddress;
    bp->bpMultiple    = rbp.bpMultiple;
    bp->bpEnabled     = rbp.bpEnabled;
    setState(id, BreakpointOk);
}
#endif

} // namespace Internal
} // namespace Debugger
