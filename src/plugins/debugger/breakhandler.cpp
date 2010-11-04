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
#include "debuggerengine.h"
#include "debuggerplugin.h"
#include "debuggerstringutils.h"
#include "threadshandler.h"

#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

namespace Debugger {
namespace Internal {

static DebuggerPlugin *plugin() { return DebuggerPlugin::instance(); }


static Breakpoints m_bp;
// FIXME this is only static because it might map to bps in the above list
static QHash<quint64,BreakpointMarker*> m_markers;

//////////////////////////////////////////////////////////////////
//
// BreakHandler
//
//////////////////////////////////////////////////////////////////

BreakHandler::BreakHandler()
  : m_breakpointIcon(_(":/debugger/images/breakpoint_16.png")),
    m_disabledBreakpointIcon(_(":/debugger/images/breakpoint_disabled_16.png")),
    m_pendingBreakPointIcon(_(":/debugger/images/breakpoint_pending_16.png")),
    //m_emptyIcon(_(":/debugger/images/watchpoint.png")),
    m_emptyIcon(_(":/debugger/images/breakpoint_pending_16.png")),
    //m_emptyIcon(_(":/debugger/images/debugger_empty_14.png")),
    m_watchpointIcon(_(":/debugger/images/watchpoint.png")),
    m_lastFound(0),
    m_lastFoundQueried(false)
{}

BreakHandler::~BreakHandler()
{
    clear();
}

int BreakHandler::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 8;
}

int BreakHandler::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : size();
}

bool BreakHandler::hasPendingBreakpoints() const
{
    for (int i = size() - 1; i >= 0; i--)
        if (at(i)->pending)
            return true;
    return false;
}

BreakpointData *BreakHandler::at(int index) const
{
    QTC_ASSERT(index < size(), return 0);
    return m_bp.at(index);
}

void BreakHandler::removeAt(int index)
{
    BreakpointData *data = at(index);
    QTC_ASSERT(data, return);
    m_bp.removeAt(index);
    delete m_markers.take(data->id);
    delete data;
}

void BreakHandler::clear()
{
    m_enabled.clear();
    m_disabled.clear();
    m_removed.clear();
}

BreakpointData *BreakHandler::findSimilarBreakpoint(const BreakpointData *needle) const
{
    // Search a breakpoint we might refer to.
    for (int index = 0; index != size(); ++index) {
        BreakpointData *data = m_bp[index];
        if (data->isSimilarTo(needle))
            return data;
    }
    return 0;
}

BreakpointData *BreakHandler::findBreakpointByNumber(int bpNumber) const
{
    if (!size())
        return 0;
    QString numStr = QString::number(bpNumber);
    for (int index = 0; index != size(); ++index)
        if (at(index)->bpNumber == numStr)
            return at(index);
    return 0;
}

int BreakHandler::findWatchPointIndexByAddress(quint64 address) const
{
    for (int index = size() - 1; index >= 0; --index) {
        BreakpointData *bd = at(index);
        if (bd->isWatchpoint() && bd->address == address)
            return index;
    }
    return -1;
}

bool BreakHandler::watchPointAt(quint64 address) const
{
    return findWatchPointIndexByAddress(address) != -1;
}

void BreakHandler::saveBreakpoints()
{
    //qDebug() << "SAVING BREAKPOINTS...";
    QTC_ASSERT(plugin(), return);
    QList<QVariant> list;
    for (int index = 0; index != size(); ++index) {
        const BreakpointData *data = at(index);
        QMap<QString, QVariant> map;
        // Do not persist Watchpoints.
        //if (data->isWatchpoint())
        //    continue;
        if (data->type != BreakpointByFileAndLine)
            map.insert(_("type"), data->type);
        if (!data->fileName.isEmpty())
            map.insert(_("filename"), data->fileName);
        if (data->lineNumber)
            map.insert(_("linenumber"), QVariant(data->lineNumber));
        if (!data->funcName.isEmpty())
            map.insert(_("funcname"), data->funcName);
        if (data->address)
            map.insert(_("address"), data->address);
        if (!data->condition.isEmpty())
            map.insert(_("condition"), data->condition);
        if (data->ignoreCount)
            map.insert(_("ignorecount"), QVariant(data->ignoreCount));
        if (!data->threadSpec.isEmpty())
            map.insert(_("threadspec"), data->threadSpec);
        if (!data->enabled)
            map.insert(_("disabled"), _("1"));
        if (data->useFullPath)
            map.insert(_("usefullpath"), _("1"));
        list.append(map);
    }
    plugin()->setSessionValue("Breakpoints", list);
    //qDebug() << "SAVED BREAKPOINTS" << this << list.size();
}

void BreakHandler::loadBreakpoints()
{
    QTC_ASSERT(plugin(), return);
    //qDebug() << "LOADING BREAKPOINTS...";
    QVariant value = plugin()->sessionValue("Breakpoints");
    QList<QVariant> list = value.toList();
    clear();
    foreach (const QVariant &var, list) {
        const QMap<QString, QVariant> map = var.toMap();
        BreakpointData *data = new BreakpointData;
        QVariant v = map.value(_("filename"));
        if (v.isValid())
            data->fileName = v.toString();
        v = map.value(_("linenumber"));
        if (v.isValid())
            data->lineNumber = v.toString().toInt();
        v = map.value(_("condition"));
        if (v.isValid())
            data->condition = v.toString().toLatin1();
        v = map.value(_("address"));
        if (v.isValid())
            data->address = v.toString().toULongLong();
        v = map.value(_("ignorecount"));
        if (v.isValid())
            data->ignoreCount = v.toString().toInt();
        v = map.value(_("threadspec"));
        if (v.isValid())
            data->threadSpec = v.toString().toLatin1();
        v = map.value(_("funcname"));
        if (v.isValid())
            data->funcName = v.toString();
        v = map.value(_("disabled"));
        if (v.isValid())
            data->enabled = !v.toInt();
        v = map.value(_("usefullpath"));
        if (v.isValid())
            data->useFullPath = bool(v.toInt());
        v = map.value(_("type"));
        if (v.isValid())
            data->type = BreakpointType(v.toInt());
        data->setMarkerFileName(data->fileName);
        data->setMarkerLineNumber(data->lineNumber);
        append(data);
    }
    //qDebug() << "LOADED BREAKPOINTS" << this << list.size();
}

void BreakHandler::updateMarkers()
{
    for (int index = 0; index != size(); ++index)
        updateMarker(at(index));
    emit layoutChanged();
}

void BreakHandler::updateMarker(BreakpointData * bp)
{
    BreakpointMarker *marker = m_markers.value(bp->id);

    if (marker && (bp->m_markerFileName != marker->fileName()
                || bp->m_markerLineNumber != marker->lineNumber())) {
        removeMarker(bp);
        marker = 0;
    }

    if (!marker && !bp->m_markerFileName.isEmpty() && bp->m_markerLineNumber > 0) {
        marker = new BreakpointMarker(this, bp, bp->m_markerFileName, bp->m_markerLineNumber);
        m_markers.insert(bp->id, marker);
    }

    if (marker)
        marker->setPending(bp->pending);
}
void BreakHandler::removeMarker(BreakpointData * bp)
{
    delete m_markers.take(bp->id);
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

QVariant BreakHandler::data(const QModelIndex &mi, int role) const
{
    static const QString empty = QString(QLatin1Char('-'));

    QTC_ASSERT(mi.isValid(), return QVariant());
    if (mi.row() >= size())
        return QVariant();

    BreakpointData *data = at(mi.row());

    switch (mi.column()) {
        case 0:
            if (role == Qt::DisplayRole) {
                const QString str = data->bpNumber;
                return str.isEmpty() ? empty : str;
            }
            if (role == Qt::DecorationRole) {
                if (data->isWatchpoint())
                    return m_watchpointIcon;
                if (!data->enabled)
                    return m_disabledBreakpointIcon;
                return data->pending ? m_pendingBreakPointIcon : m_breakpointIcon;
            }
            break;
        case 1:
            if (role == Qt::DisplayRole) {
                const QString str = data->pending ? data->funcName : data->bpFuncName;
                return str.isEmpty() ? empty : str;
            }
            break;
        case 2:
            if (role == Qt::DisplayRole) {
                QString str = data->pending ? data->fileName : data->bpFileName;
                str = QFileInfo(str).fileName();
                // FIXME: better?
                //if (data->bpMultiple && str.isEmpty() && !data->markerFileName.isEmpty())
                //    str = data->markerFileName;
                str = str.isEmpty() ? empty : str;
                if (data->useFullPath)
                    str = QDir::toNativeSeparators(QLatin1String("/.../") + str);
                return str;
            }
            break;
        case 3:
            if (role == Qt::DisplayRole) {
                // FIXME: better?
                //if (data->bpMultiple && str.isEmpty() && !data->markerFileName.isEmpty())
                //    str = data->markerLineNumber;
                const int nr = data->pending ? data->lineNumber : data->bpLineNumber;
                return nr ? QString::number(nr) : empty;
            }
            if (role == Qt::UserRole + 1)
                return data->lineNumber;
            break;
        case 4:
            if (role == Qt::DisplayRole)
                return data->pending ? data->condition : data->bpCondition;
            if (role == Qt::ToolTipRole)
                return tr("Breakpoint will only be hit if this condition is met.");
            if (role == Qt::UserRole + 1)
                return data->condition;
            break;
        case 5:
            if (role == Qt::DisplayRole) {
                const int ignoreCount = data->pending ? data->ignoreCount : data->bpIgnoreCount;
                return ignoreCount ? QVariant(ignoreCount) : QVariant(QString());
            }
            if (role == Qt::ToolTipRole)
                return tr("Breakpoint will only be hit after being ignored so many times.");
            if (role == Qt::UserRole + 1)
                return data->ignoreCount;
            break;
        case 6:
            if (role == Qt::DisplayRole) {
                if (data->pending)
                    return !data->threadSpec.isEmpty() ? data->threadSpec : tr("(all)");
                else
                    return !data->bpThreadSpec.isEmpty() ? data->bpThreadSpec : tr("(all)");
            }
            if (role == Qt::ToolTipRole)
                return tr("Breakpoint will only be hit in the specified thread(s).");
            if (role == Qt::UserRole + 1)
                return data->threadSpec;
            break;
        case 7:
            if (role == Qt::DisplayRole) {
                QString displayValue;
                const quint64 effectiveAddress
                    = data->isWatchpoint() ? data->address : data->bpAddress;
                if (effectiveAddress)
                    displayValue += QString::fromAscii("0x%1").arg(effectiveAddress, 0, 16);
                if (!data->bpState.isEmpty()) {
                    if (!displayValue.isEmpty())
                        displayValue += QLatin1Char(' ');
                    displayValue += QString::fromAscii(data->bpState);
                }
                return displayValue;
            }
            break;
    }
    if (role == Qt::ToolTipRole)
        return theDebuggerBoolSetting(UseToolTipsInBreakpointsView)
                ? data->toToolTip() : QVariant();
    return QVariant();
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

void BreakHandler::reinsertBreakpoint(BreakpointData *data)
{
    // FIXME: Use some more direct method?
    appendBreakpoint(data->clone());
    removeBreakpoint(data);
    synchronizeBreakpoints();
    emit layoutChanged();
}

void BreakHandler::append(BreakpointData *data)
{
    m_bp.append(data);
}

Breakpoints BreakHandler::takeRemovedBreakpoints()
{
    foreach(BreakpointData *bp, m_removed) {
        removeMarker(bp);
    }
    Breakpoints result = m_removed;
    m_removed.clear();
    return result;
}

Breakpoints BreakHandler::takeEnabledBreakpoints()
{
    Breakpoints result = m_enabled;
    m_enabled.clear();
    return result;
}

Breakpoints BreakHandler::takeDisabledBreakpoints()
{
    Breakpoints result = m_disabled;
    m_disabled.clear();
    return result;
}

void BreakHandler::removeBreakpointHelper(int index)
{
    BreakpointData *data = at(index);
    QTC_ASSERT(data, return);
    m_bp.removeAt(index);
    removeMarker(data);
    m_removed.append(data);
}

void BreakHandler::removeBreakpoint(int index)
{
    if (index < 0 || index >= size())
        return;
    removeBreakpointHelper(index);
    emit layoutChanged();
}

void BreakHandler::removeBreakpoint(BreakpointData *data)
{
    removeBreakpointHelper(indexOf(data));
    emit layoutChanged();
}

void BreakHandler::toggleBreakpointEnabled(BreakpointData *data)
{
    QTC_ASSERT(data, return);
    data->enabled = !data->enabled;
    if (data->enabled) {
        m_enabled.append(data);
        m_disabled.removeAll(data);
    } else {
        m_enabled.removeAll(data);
        m_disabled.append(data);
    }
    removeMarker(data); // Force icon update.
    updateMarker(data);
    emit layoutChanged();
    synchronizeBreakpoints();
}

void BreakHandler::toggleBreakpointEnabled(const QString &fileName, int lineNumber)
{
    BreakpointData *data = findBreakpoint(fileName, lineNumber);
    toggleBreakpointEnabled(data);
}

void BreakHandler::appendBreakpoint(BreakpointData *data)
{
    append(data);
    emit layoutChanged();
    saveBreakpoints();  // FIXME: remove?
    updateMarkers();
}

void BreakHandler::removeAllBreakpoints()
{
    for (int index = size(); --index >= 0;)
        removeBreakpointHelper(index);
    emit layoutChanged();
    updateMarkers();
}

BreakpointData *BreakHandler::findBreakpoint(quint64 address) const
{
    foreach (BreakpointData *data, m_bp)
        if (data->address == address)
            return data;
    return 0;
}

BreakpointData *BreakHandler::findBreakpoint(const QString &fileName,
    int lineNumber, bool useMarkerPosition)
{
    foreach (BreakpointData *data, m_bp)
        if (data->isLocatedAt(fileName, lineNumber, useMarkerPosition))
            return data;
    return 0;
}

void BreakHandler::toggleBreakpoint(const QString &fileName, int lineNumber,
                                    quint64 address /* = 0 */)
{
    BreakpointData *data = 0;

    if (address) {
        data = findBreakpoint(address);
    } else {
        data = findBreakpoint(fileName, lineNumber, true);
        if (!data)
            data = findBreakpoint(fileName, lineNumber, false);
    }

    if (data) {
        removeBreakpoint(data);
    } else {
        data = new BreakpointData;
        if (address) {
            data->address = address;
        } else {
            data->fileName = fileName;
            data->lineNumber = lineNumber;
        }
        data->pending = true;
        data->setMarkerFileName(fileName);
        data->setMarkerLineNumber(lineNumber);
        appendBreakpoint(data);
    }
    synchronizeBreakpoints();
}

void BreakHandler::saveSessionData()
{
    // FIXME QTC_ASSERT(m_engine->isSessionEngine(), return);
    saveBreakpoints();
}

void BreakHandler::loadSessionData()
{
    // FIXME QTC_ASSERT(m_engine->isSessionEngine(), return);
    foreach (BreakpointData *bp, m_bp) {
        delete m_markers.take(bp->id);
    }
    qDeleteAll(m_bp);
    m_bp.clear();
    loadBreakpoints();
    updateMarkers();
}

void BreakHandler::breakByFunction(const QString &functionName)
{
    // One breakpoint per function is enough for now. This does not handle
    // combinations of multiple conditions and ignore counts, though.
    for (int index = size(); --index >= 0;) {
        const BreakpointData *data = at(index);
        QTC_ASSERT(data, break);
        if (data->funcName == functionName && data->condition.isEmpty()
                && data->ignoreCount == 0)
            return;
    }
    BreakpointData *data = new BreakpointData;
    data->funcName = functionName;
    append(data);
    //saveBreakpoints();
    updateMarkers();
}

bool BreakHandler::isActive() const
{
    return true; // FIXME m_engine->isActive();
}

void BreakHandler::initializeFromTemplate(BreakHandler *other)
{
    Q_UNUSED(other)
    m_inserted.clear();
    foreach (BreakpointData *data, m_bp) {
        if (true /* FIXME m_engine->acceptsBreakpoint(data) */) {
            BreakpointMarker *marker = m_markers.value(data->id);
            if (marker)
                marker->m_handler = this;
            m_inserted.append(data);
        }
    }
}

void BreakHandler::storeToTemplate(BreakHandler *other)
{
    foreach (BreakpointData *data, m_bp) {
        BreakpointMarker *marker = m_markers.value(data->id);
        if (marker)
            marker->m_handler = other;
        data->clear();
    }
    other->saveSessionData();
}

int BreakHandler::size() const
{
    return m_bp.size();
}

int BreakHandler::indexOf(BreakpointData *data)
{
    return m_bp.indexOf(data);
}

void BreakHandler::synchronizeBreakpoints()
{
    emit breakpointSynchronizationRequested();
}

} // namespace Internal
} // namespace Debugger
