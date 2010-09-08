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

#include "debuggeractions.h"
#include "debuggerengine.h"
#include "debuggerplugin.h"
#include "debuggerstringutils.h"
#include "threadshandler.h"
#include "stackhandler.h"
#include "stackframe.h"

#include <texteditor/basetextmark.h>
#include <utils/qtcassert.h>

#include <QtCore/QByteArray>
#include <QtCore/QFileInfo>


namespace Debugger {
namespace Internal {

static DebuggerPlugin *plugin() { return DebuggerPlugin::instance(); }


//////////////////////////////////////////////////////////////////
//
// BreakHandler
//
//////////////////////////////////////////////////////////////////

BreakHandler::BreakHandler(DebuggerEngine *engine)
  : m_breakpointIcon(_(":/debugger/images/breakpoint_16.png")),
    m_disabledBreakpointIcon(_(":/debugger/images/breakpoint_disabled_16.png")),
    m_pendingBreakPointIcon(_(":/debugger/images/breakpoint_pending_16.png")),
    //m_emptyIcon(_(":/debugger/images/watchpoint.png")),
    m_emptyIcon(_(":/debugger/images/breakpoint_pending_16.png")),
    //m_emptyIcon(_(":/debugger/images/debugger_empty_14.png")),
    m_watchpointIcon(_(":/debugger/images/watchpoint.png")),
    m_engine(engine),
    m_lastFound(0),
    m_lastFoundQueried(false)
{
    QTC_ASSERT(m_engine, /**/);
}

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
    m_bp.removeAt(index);
    delete data;
}

void BreakHandler::clear()
{
    qDeleteAll(m_bp);
    m_bp.clear();
    m_enabled.clear();
    m_disabled.clear();
    m_removed.clear();
    m_inserted.clear();
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

int BreakHandler::findWatchPointIndexByAddress(const QByteArray &a) const
{
    for (int index = size() - 1; index >= 0; --index) {
        BreakpointData *bd = at(index);
        if (bd->type == BreakpointData::WatchpointType && bd->address == a)
            return index;
    }
    return -1;
}

bool BreakHandler::watchPointAt(quint64 address) const
{
    const QByteArray addressBA = QByteArray("0x") + QByteArray::number(address, 16);
    return findWatchPointIndexByAddress(addressBA) != -1;
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
        //if (data->type == BreakpointData::WatchpointType)
        //    continue;
        if (data->type != BreakpointData::BreakpointType)
            map.insert(_("type"), data->type);
        if (!data->fileName.isEmpty())
            map.insert(_("filename"), data->fileName);
        if (!data->lineNumber.isEmpty())
            map.insert(_("linenumber"), data->lineNumber);
        if (!data->funcName.isEmpty())
            map.insert(_("funcname"), data->funcName);
        if (!data->address.isEmpty())
            map.insert(_("address"), data->address);
        if (!data->condition.isEmpty())
            map.insert(_("condition"), data->condition);
        if (!data->ignoreCount.isEmpty())
            map.insert(_("ignorecount"), data->ignoreCount);
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
            data->lineNumber = v.toString().toLatin1();
        v = map.value(_("condition"));
        if (v.isValid())
            data->condition = v.toString().toLatin1();
        v = map.value(_("address"));
        if (v.isValid())
            data->address = v.toString().toLatin1();
        v = map.value(_("ignorecount"));
        if (v.isValid())
            data->ignoreCount = v.toString().toLatin1();
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
            data->type = BreakpointData::Type(v.toInt());
        data->setMarkerFileName(data->fileName);
        data->setMarkerLineNumber(data->lineNumber.toInt());
        append(data);
    }
    //qDebug() << "LOADED BREAKPOINTS" << this << list.size();
}

void BreakHandler::updateMarkers()
{
    for (int index = 0; index != size(); ++index)
        at(index)->updateMarker();
    emit layoutChanged();
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

    switch (role) {
        case CurrentThreadIdRole:
            QTC_ASSERT(m_engine, return QVariant());
            return m_engine->threadsHandler()->currentThreadId();

        case EngineActionsEnabledRole:
            QTC_ASSERT(m_engine, return QVariant());
            return m_engine->debuggerActionsEnabled();

        case EngineCapabilitiesRole:
            QTC_ASSERT(m_engine, return QVariant());
            return m_engine->debuggerCapabilities();

        default:
            break;
    }

    QTC_ASSERT(mi.isValid(), return QVariant());
    if (mi.row() >= size())
        return QVariant();

    const BreakpointData *data = at(mi.row());

    if (role == BreakpointUseFullPathRole)
        return data->useFullPath;

    if (role == BreakpointFileNameRole)
        return data->fileName;

    if (role == BreakpointEnabledRole)
        return data->enabled;

    if (role == BreakpointFunctionNameRole)
        return data->funcName;

    if (role == BreakpointConditionRole)
        return data->condition;

    if (role == BreakpointIgnoreCountRole)
        return data->ignoreCount;

    if (role == BreakpointThreadSpecRole)
        return data->threadSpec;

    switch (mi.column()) {
        case 0:
            if (role == Qt::DisplayRole) {
                const QString str = data->bpNumber;
                return str.isEmpty() ? empty : str;
            }
            if (role == Qt::DecorationRole) {
                if (data->type == BreakpointData::WatchpointType)
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
                    str = "/.../" + str;
                return str;
            }
            break;
        case 3:
            if (role == Qt::DisplayRole) {
                // FIXME: better?
                //if (data->bpMultiple && str.isEmpty() && !data->markerFileName.isEmpty())
                //    str = data->markerLineNumber;
                const QString str = data->pending ? data->lineNumber : data->bpLineNumber;
                return str.isEmpty() ? empty : str;
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
            if (role == Qt::DisplayRole)
                return data->pending ? data->ignoreCount : data->bpIgnoreCount;
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
                if (data->type == BreakpointData::WatchpointType)
                    return data->address;
                return data->bpAddress;
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

bool BreakHandler::setData(const QModelIndex &index, const QVariant &value, int role)
{
    switch (role) {
        case RequestActivateBreakpointRole: {
            const BreakpointData *data = at(value.toInt());
            QTC_ASSERT(data, return false);
            m_engine->gotoLocation(data->markerFileName(),
                data->markerLineNumber(), false);
            return true;
        }

        case RequestRemoveBreakpointByIndexRole: {
            BreakpointData *data = at(value.toInt());
            QTC_ASSERT(data, return false);
            removeBreakpoint(data);
            return true;
        }

        case RequestSynchronizeBreakpointsRole:
            QTC_ASSERT(m_engine, return false);
            m_engine->attemptBreakpointSynchronization();
            return true;

        case RequestBreakByFunctionRole:
            QTC_ASSERT(m_engine, return false);
            m_engine->breakByFunction(value.toString());
            return true;

        case RequestBreakByFunctionMainRole:
            QTC_ASSERT(m_engine, return false);
            m_engine->breakByFunctionMain();
            return true;
    }

    BreakpointData *data = at(index.row());

    switch (role) {
        case BreakpointEnabledRole:
            if (data->enabled != value.toBool())
                toggleBreakpointEnabled(data);
            return true;

        case BreakpointUseFullPathRole:
            if (data->useFullPath != value.toBool()) {
                data->useFullPath = value.toBool();
                emit layoutChanged();
            }
            return true;

        case BreakpointConditionRole: {
                QByteArray val = value.toString().toLatin1();
                if (val != data->condition) {
                    data->condition = val;
                    emit layoutChanged();
                }
            }
            return true;

        case BreakpointIgnoreCountRole: {
                QByteArray val = value.toString().toLatin1();
                if (val != data->ignoreCount) {
                    data->ignoreCount = val;
                    emit layoutChanged();
                }
            }
            return true;

        case BreakpointThreadSpecRole: {
                QByteArray val = value.toString().toLatin1();
                if (val != data->threadSpec) {
                    data->threadSpec = val;
                    emit layoutChanged();
                }
            }
            return true;
    }
    return false;
}

void BreakHandler::append(BreakpointData *data)
{
    data->m_handler = this;
    m_bp.append(data);
    m_inserted.append(data);
}

Breakpoints BreakHandler::insertedBreakpoints() const
{
    return m_inserted;
}

void BreakHandler::takeInsertedBreakPoint(BreakpointData *d)
{
    m_inserted.removeAll(d);
}

Breakpoints BreakHandler::takeRemovedBreakpoints()
{
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
    BreakpointData *data = m_bp.at(index);
    m_bp.removeAt(index);
    data->removeMarker();
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
    removeBreakpointHelper(m_bp.indexOf(data));
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
    data->updateMarker();
    emit layoutChanged();
    m_engine->attemptBreakpointSynchronization();
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

BreakpointData *BreakHandler::findBreakpoint(const QString &fileName,
    int lineNumber, bool useMarkerPosition)
{
    foreach (BreakpointData *data, m_bp)
        if (data->isLocatedAt(fileName, lineNumber, useMarkerPosition))
            return data;
    return 0;
}

void BreakHandler::toggleBreakpoint(const QString &fileName, int lineNumber)
{
    BreakpointData *data = findBreakpoint(fileName, lineNumber, true);
    if (!data)
        data = findBreakpoint(fileName, lineNumber, false);
    if (data) {
        removeBreakpoint(data);
    } else {
        data = new BreakpointData;
        data->fileName = fileName;
        data->lineNumber = QByteArray::number(lineNumber);
        data->pending = true;
        data->setMarkerFileName(fileName);
        data->setMarkerLineNumber(lineNumber);
        appendBreakpoint(data);
    }
    m_engine->attemptBreakpointSynchronization();
}

void BreakHandler::saveSessionData()
{
    QTC_ASSERT(m_engine->isSessionEngine(), return);
    saveBreakpoints();
}

void BreakHandler::loadSessionData()
{
    QTC_ASSERT(m_engine->isSessionEngine(), return);
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
                && data->ignoreCount.isEmpty())
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
    return m_engine->isActive();
}

void BreakHandler::initializeFromTemplate(BreakHandler *other)
{
    //qDebug() << "COPYING BREAKPOINTS INTO NEW SESSION";
    QTC_ASSERT(m_bp.isEmpty(), /**/);
    foreach (BreakpointData *data, other->m_bp) {
        append(data->clone());
        data->removeMarker();
    }
    updateMarkers();
}

void BreakHandler::storeToTemplate(BreakHandler *other)
{
    other->removeAllBreakpoints();
    foreach (BreakpointData *data, m_bp)
        other->append(data->clone());
    removeAllBreakpoints();
    other->updateMarkers();
    other->saveSessionData();
}

} // namespace Internal
} // namespace Debugger
