/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "breakhandler.h"
#include "debuggerinternalconstants.h"
#include "breakpointmarker.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerstringutils.h"
#include "stackframe.h"

#include <extensionsystem/invoker.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#if USE_BREAK_MODEL_TEST
#include "modeltest.h"
#endif

#include <QDir>
#include <QFileInfo>
#include <QTimerEvent>

#define BREAK_ASSERT(cond, action) if (cond) {} else { action; }
//#define BREAK_ASSERT(cond, action) QTC_ASSERT(cond, action)

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
    return BreakHandler::tr("Breakpoint at \"%1\"").arg(QString::fromLatin1(func));
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
        //case BreakpointAtVFork:
        //    return msgBreakpointAtSpecialFunc("vfork");
        case BreakpointAtSysCall:
            return msgBreakpointAtSpecialFunc("syscall");
        case BreakpointAtMain:
            return BreakHandler::tr("Breakpoint at Function \"main()\"");
        case WatchpointAtAddress:
            return BreakHandler::tr("Watchpoint at Address");
        case WatchpointAtExpression:
            return BreakHandler::tr("Watchpoint at Expression");
        case BreakpointOnQmlSignalEmit:
            return BreakHandler::tr("Breakpoint on QML Signal Emit");
        case BreakpointAtJavaScriptThrow:
            return BreakHandler::tr("Breakpoint at JavaScript throw");
        case UnknownType:
            break;
    }
    return BreakHandler::tr("Unknown Breakpoint Type");
}

BreakHandler::BreakHandler()
  : m_syncTimerId(-1)
{
#if USE_BREAK_MODEL_TEST
    new ModelTest(this, 0);
#endif
}

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

static inline bool fileNameMatch(const QString &f1, const QString &f2)
{
    if (Utils::HostOsInfo::isWindowsHost())
        return f1.compare(f2, Qt::CaseInsensitive) == 0;
    return f1 == f2;
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

BreakpointModelId BreakHandler::findSimilarBreakpoint(const BreakpointResponse &needle) const
{
    // Search a breakpoint we might refer to.
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it) {
        const BreakpointModelId id = it.key();
        const BreakpointParameters &data = it->data;
        const BreakpointResponse &response = it->response;
        //qDebug() << "COMPARING " << data.toString() << " WITH " << needle.toString();
        if (response.id.isValid() && response.id.majorPart() == needle.id.majorPart())
            return id;

        if (isSimilarTo(data, needle))
            return id;
    }
    return BreakpointModelId();
}

BreakpointModelId BreakHandler::findBreakpointByResponseId(const BreakpointResponseId &id) const
{
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it)
        if (it->response.id.majorPart() == id.majorPart())
            return it.key();
    return BreakpointModelId();
}

BreakpointModelId BreakHandler::findBreakpointByFunction(const QString &functionName) const
{
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it)
        if (it->data.functionName == functionName)
            return it.key();
    return BreakpointModelId();
}

BreakpointModelId BreakHandler::findBreakpointByAddress(quint64 address) const
{
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it)
        if (it->data.address == address || it->response.address == address)
            return it.key();
    return BreakpointModelId();
}

BreakpointModelId BreakHandler::findBreakpointByFileAndLine(const QString &fileName,
    int lineNumber, bool useMarkerPosition)
{
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it)
        if (it->isLocatedAt(fileName, lineNumber, useMarkerPosition))
            return it.key();
    return BreakpointModelId();
}

const BreakpointParameters &BreakHandler::breakpointData(BreakpointModelId id) const
{
    static BreakpointParameters dummy;
    ConstIterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return dummy);
    return it->data;
}

BreakpointModelId BreakHandler::findWatchpoint(const BreakpointParameters &data) const
{
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it)
        if (it->data.isWatchpoint()
                && it->data.address == data.address
                && it->data.size == data.size
                && it->data.expression == data.expression
                && it->data.bitpos == data.bitpos)
            return it.key();
    return BreakpointModelId();
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
        if (data.oneShot)
            map.insert(_("oneshot"), one);
        if (data.pathUsage != BreakpointPathUsageEngineDefault)
            map.insert(_("usefullpath"), QString::number(data.pathUsage));
        if (data.tracepoint)
            map.insert(_("tracepoint"), one);
        if (!data.module.isEmpty())
            map.insert(_("module"), data.module);
        if (!data.command.isEmpty())
            map.insert(_("command"), data.command);
        if (!data.expression.isEmpty())
            map.insert(_("expression"), data.expression);
        if (!data.message.isEmpty())
            map.insert(_("message"), data.message);
        list.append(map);
    }
    debuggerCore()->setSessionValue(QLatin1String("Breakpoints"), list);
    //qDebug() << "SAVED BREAKPOINTS" << this << list.size();
}

void BreakHandler::loadBreakpoints()
{
    QTC_ASSERT(debuggerCore(), return);
    //qDebug() << "LOADING BREAKPOINTS...";
    QVariant value = debuggerCore()->sessionValue(QLatin1String("Breakpoints"));
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
        v = map.value(_("oneshot"));
        if (v.isValid())
            data.oneShot = v.toInt();
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
        v = map.value(_("expression"));
        if (v.isValid())
            data.expression = v.toString();
        v = map.value(_("message"));
        if (v.isValid())
            data.message = v.toString();
        if (data.isValid())
            appendBreakpoint(data);
        else
            qWarning("Not restoring invalid breakpoint: %s", qPrintable(data.toString()));
    }
    //qDebug() << "LOADED BREAKPOINTS" << this << list.size();
}

void BreakHandler::updateMarkers()
{
    Iterator it = m_storage.begin(), et = m_storage.end();
    for ( ; it != et; ++it)
        it->updateMarker(it.key());
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

BreakpointModelId BreakHandler::findBreakpointByIndex(const QModelIndex &index) const
{
    //qDebug() << "FIND: " << index <<
    //    BreakpointId::fromInternalId(index.internalId());
    return BreakpointModelId::fromInternalId(index.internalId());
}

BreakpointModelIds BreakHandler::findBreakpointsByIndex(const QList<QModelIndex> &list) const
{
    QSet<BreakpointModelId> ids;
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
            return QAbstractItemModel::flags(index);
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

QModelIndex BreakHandler::createIndex(int row, int column, quint32 id) const
{
    return QAbstractItemModel::createIndex(row, column, id);
}

QModelIndex BreakHandler::createIndex(int row, int column, void *ptr) const
{
    QTC_CHECK(false); // This function is not used.
    return QAbstractItemModel::createIndex(row, column, ptr);
}

int BreakHandler::columnCount(const QModelIndex &idx) const
{
    if (idx.column() > 0)
        return 0;
    const BreakpointModelId id = findBreakpointByIndex(idx);
    return id.isMinor() ? 0 : 8;
}

int BreakHandler::rowCount(const QModelIndex &idx) const
{
    if (idx.column() > 0)
        return 0;
    if (!idx.isValid())
        return m_storage.size();
    const BreakpointModelId id = findBreakpointByIndex(idx);
    if (id.isMajor())
        return m_storage.value(id).subItems.size();
    return 0;
}

QModelIndex BreakHandler::index(int row, int col, const QModelIndex &parent) const
{
    if (row < 0 || col < 0)
        return QModelIndex();
    if (parent.column() > 0)
        return QModelIndex();
    BreakpointModelId id = findBreakpointByIndex(parent);
    if (id.isMajor()) {
        ConstIterator it = m_storage.find(id);
        if (row >= it->subItems.size())
            return QModelIndex();
        BreakpointModelId sub = id.child(row);
        return createIndex(row, col, sub.toInternalId());
    }
    if (id.isMinor())
        return QModelIndex();
    QTC_ASSERT(!id.isValid(), return QModelIndex());
    if (row >= m_storage.size())
        return QModelIndex();
    id = at(row);
    return createIndex(row, col, id.toInternalId());
}

QModelIndex BreakHandler::parent(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QModelIndex();
    BreakpointModelId id = findBreakpointByIndex(idx);
    if (id.isMajor())
        return QModelIndex();
    if (id.isMinor()) {
        BreakpointModelId pid = id.parent();
        int row = indexOf(pid);
        return createIndex(row, 0, pid.toInternalId());
    }
    return QModelIndex();
}

QVariant BreakHandler::data(const QModelIndex &mi, int role) const
{
    static const QString empty = QString(QLatin1Char('-'));

    if (!mi.isValid())
        return QVariant();

    BreakpointModelId id = findBreakpointByIndex(mi);

    BreakpointModelId pid = id;
    if (id.isMinor())
        pid = id.parent();

    ConstIterator it = m_storage.find(pid);
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

    if (id.isMinor()) {
        QTC_ASSERT(id.minorPart() <= it->subItems.size(), return QVariant());
        const BreakpointResponse &res = it->subItems.at(id.minorPart() - 1);
        switch (mi.column()) {
        case 0:
            if (role == Qt::DisplayRole)
                return id.toString();
        case 1:
            if (role == Qt::DisplayRole)
                return res.functionName;
        case 4:
            if (role == Qt::DisplayRole)
                if (res.address)
                    return QString::fromLatin1("0x%1").arg(res.address, 0, 16);
        }
        return QVariant();
    }

    switch (mi.column()) {
        case 0:
            if (role == Qt::DisplayRole)
                return id.toString();
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
                        //|| data.type == BreakpointAtVFork
                        || data.type == BreakpointAtSysCall)
                    return typeToString(data.type);
                if (data.type == WatchpointAtAddress) {
                    quint64 address = response.address ? response.address : data.address;
                    return tr("Data at 0x%1").arg(address, 0, 16);
                }
                if (data.type == WatchpointAtExpression) {
                    QString expression = !response.expression.isEmpty()
                            ? response.expression : data.expression;
                    return tr("Data at %1").arg(expression);
                }
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
                const quint64 address = orig ? data.address : response.address;
                if (address)
                    return QString::fromLatin1("0x%1").arg(address, 0, 16);
                return QVariant();
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
    }
    return QVariant();
}

#define GETTER(type, getter) \
type BreakHandler::getter(BreakpointModelId id) const \
{ \
    ConstIterator it = m_storage.find(id); \
    BREAK_ASSERT(it != m_storage.end(), \
        qDebug() << "ID" << id << "NOT KNOWN"; \
        return type()); \
    return it->data.getter; \
}

#define SETTER(type, getter, setter) \
void BreakHandler::setter(BreakpointModelId id, const type &value) \
{ \
    Iterator it = m_storage.find(id); \
    BREAK_ASSERT(it != m_storage.end(), \
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
PROPERTY(QString, expression, setExpression)
PROPERTY(QString, message, setMessage)
PROPERTY(int, ignoreCount, setIgnoreCount)

bool BreakHandler::isEnabled(BreakpointModelId id) const
{
    ConstIterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return false);
    return it->data.enabled;
}

void BreakHandler::setEnabled(BreakpointModelId id, bool on)
{
    Iterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return);
    //qDebug() << "SET ENABLED: " << id << it->data.isEnabled() << on;
    if (it->data.enabled == on)
        return;
    it->data.enabled = on;
    it->updateMarkerIcon();
    if (it->engine) {
        it->state = BreakpointChangeRequested;
        scheduleSynchronization();
    }
}

bool BreakHandler::isWatchpoint(BreakpointModelId id) const
{
    ConstIterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return false);
    return it->data.isWatchpoint();
}

bool BreakHandler::isTracepoint(BreakpointModelId id) const
{
    ConstIterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return false);
    return it->data.tracepoint;
}

bool BreakHandler::isOneShot(BreakpointModelId id) const
{
    ConstIterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return false);
    return it->data.oneShot;
}

bool BreakHandler::needsChildren(BreakpointModelId id) const
{
    ConstIterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return false);
    return it->response.multiple && it->subItems.isEmpty();
}

void BreakHandler::setTracepoint(BreakpointModelId id, bool on)
{
    Iterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return);
    if (it->data.tracepoint == on)
        return;
    it->data.tracepoint = on;
    it->updateMarkerIcon();

    if (it->engine) {
        it->state = BreakpointChangeRequested;
        scheduleSynchronization();
    }
}

void BreakHandler::setMarkerFileAndLine(BreakpointModelId id,
    const QString &fileName, int lineNumber)
{
    Iterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(),
        qDebug() << "MARKER_FILE_AND_LINE: " << id; return);
    if (it->response.fileName == fileName && it->response.lineNumber == lineNumber)
        return;
    it->response.fileName = fileName;
    it->response.lineNumber = lineNumber;
    it->destroyMarker();
    it->updateMarker(id);
    emit layoutChanged();
}

BreakpointState BreakHandler::state(BreakpointModelId id) const
{
    ConstIterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(),
        qDebug() << "STATE: " << id; return BreakpointDead);
    return it->state;
}

DebuggerEngine *BreakHandler::engine(BreakpointModelId id) const
{
    ConstIterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), qDebug() << id; return 0);
    return it->engine;
}

void BreakHandler::setEngine(BreakpointModelId id, DebuggerEngine *value)
{
    Iterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), qDebug() << "SET ENGINE" << id; return);
    QTC_ASSERT(it->state == BreakpointNew, qDebug() << "STATE: " << it->state <<id);
    QTC_ASSERT(!it->engine, qDebug() << "NO ENGINE" << id; return);
    it->engine = value;
    it->state = BreakpointInsertRequested;
    it->response = BreakpointResponse();
    it->updateMarker(id);
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
            || to == BreakpointChangeRequested
            || to == BreakpointRemoveRequested;
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

bool BreakHandler::isEngineRunning(BreakpointModelId id) const
{
    if (const DebuggerEngine *e = engine(id)) {
        const DebuggerState state = e->state();
        return state != DebuggerFinished && state != DebuggerNotReady;
    }
    return false;
}

void BreakHandler::setState(BreakpointModelId id, BreakpointState state)
{
    Iterator it = m_storage.find(id);
    //qDebug() << "BREAKPOINT STATE TRANSITION, ID: " << id
    //    << " FROM: " << it->state << " TO: " << state;
    BREAK_ASSERT(it != m_storage.end(), qDebug() << id; return);
    QTC_ASSERT(isAllowedTransition(it->state, state),
        qDebug() << "UNEXPECTED BREAKPOINT STATE TRANSITION"
            << it->state << state);

    if (it->state == state) {
        qDebug() << "STATE UNCHANGED: " << id << state;
        return;
    }

    it->state = state;

    // FIXME: updateMarker() should recognize the need for icon changes.
    if (state == BreakpointInserted) {
        it->destroyMarker();
        it->updateMarker(id);
    }
    layoutChanged();
}

void BreakHandler::notifyBreakpointChangeAfterInsertNeeded(BreakpointModelId id)
{
    QTC_ASSERT(state(id) == BreakpointInsertProceeding, qDebug() << state(id));
    setState(id, BreakpointChangeRequested);
}

void BreakHandler::notifyBreakpointInsertProceeding(BreakpointModelId id)
{
    QTC_ASSERT(state(id) == BreakpointInsertRequested, qDebug() << state(id));
    setState(id, BreakpointInsertProceeding);
}

void BreakHandler::notifyBreakpointInsertOk(BreakpointModelId id)
{
    QTC_ASSERT(state(id) == BreakpointInsertProceeding, qDebug() << state(id));
    setState(id, BreakpointInserted);
    ConstIterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return);
}

void BreakHandler::notifyBreakpointInsertFailed(BreakpointModelId id)
{
    QTC_ASSERT(state(id) == BreakpointInsertProceeding, qDebug() << state(id));
    setState(id, BreakpointDead);
}

void BreakHandler::notifyBreakpointRemoveProceeding(BreakpointModelId id)
{
    QTC_ASSERT(state(id) == BreakpointRemoveRequested, qDebug() << state(id));
    setState(id, BreakpointRemoveProceeding);
}

void BreakHandler::notifyBreakpointRemoveOk(BreakpointModelId id)
{
    QTC_ASSERT(state(id) == BreakpointRemoveProceeding, qDebug() << state(id));
    setState(id, BreakpointDead);
    cleanupBreakpoint(id);
}

void BreakHandler::notifyBreakpointRemoveFailed(BreakpointModelId id)
{
    QTC_ASSERT(state(id) == BreakpointRemoveProceeding, qDebug() << state(id));
    setState(id, BreakpointDead);
    cleanupBreakpoint(id);
}

void BreakHandler::notifyBreakpointChangeProceeding(BreakpointModelId id)
{
    QTC_ASSERT(state(id) == BreakpointChangeRequested, qDebug() << state(id));
    setState(id, BreakpointChangeProceeding);
}

void BreakHandler::notifyBreakpointChangeOk(BreakpointModelId id)
{
    QTC_ASSERT(state(id) == BreakpointChangeProceeding, qDebug() << state(id));
    setState(id, BreakpointInserted);
}

void BreakHandler::notifyBreakpointChangeFailed(BreakpointModelId id)
{
    QTC_ASSERT(state(id) == BreakpointChangeProceeding, qDebug() << state(id));
    setState(id, BreakpointDead);
}

void BreakHandler::notifyBreakpointReleased(BreakpointModelId id)
{
    //QTC_ASSERT(state(id) == BreakpointChangeProceeding, qDebug() << state(id));
    Iterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return);
    it->state = BreakpointNew;
    it->engine = 0;
    it->response = BreakpointResponse();
    it->subItems.clear();
    it->destroyMarker();
    it->updateMarker(id);
    if (it->data.type == WatchpointAtAddress
            || it->data.type == WatchpointAtExpression
            || it->data.type == BreakpointByAddress)
        it->data.enabled = false;
    layoutChanged();
}

void BreakHandler::notifyBreakpointAdjusted(BreakpointModelId id,
        const BreakpointParameters &data)
{
    QTC_ASSERT(state(id) == BreakpointInserted, qDebug() << state(id));
    Iterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return);
    it->data = data;
    //if (it->needsChange())
    //    setState(id, BreakpointChangeRequested);
}

void BreakHandler::notifyBreakpointNeedsReinsertion(BreakpointModelId id)
{
    QTC_ASSERT(state(id) == BreakpointChangeProceeding, qDebug() << state(id));
    Iterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return);
    it->state = BreakpointInsertRequested;
}

void BreakHandler::removeAlienBreakpoint(BreakpointModelId id)
{
    Iterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return);
    it->state = BreakpointDead;
    cleanupBreakpoint(id);
}

void BreakHandler::removeBreakpoint(BreakpointModelId id)
{
    Iterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return);
    switch (it->state) {
    case BreakpointRemoveRequested:
        break;
    case BreakpointInserted:
    case BreakpointInsertProceeding:
        setState(id, BreakpointRemoveRequested);
        scheduleSynchronization();
        break;
    case BreakpointNew:
        it->state = BreakpointDead;
        cleanupBreakpoint(id);
        break;
    default:
        qWarning("Warning: Cannot remove breakpoint %s in state '%s'.",
               qPrintable(id.toString()), qPrintable(stateToString(it->state)));
        it->state = BreakpointRemoveRequested;
        break;
    }
}

// Ok to be not thread-safe. The order does not matter and only the gui
// produces authoritative ids.
static int currentId = 0;

void BreakHandler::appendBreakpoint(const BreakpointParameters &data)
{
    if (!data.isValid()) {
        qWarning("Not adding invalid breakpoint: %s", qPrintable(data.toString()));
        return;
    }

    BreakpointModelId id(++currentId);
    const int row = m_storage.size();
    beginInsertRows(QModelIndex(), row, row);
    Iterator it = m_storage.insert(id, BreakpointItem());
    endInsertRows();

    // Create marker after copy is inserted into hash.
    it->data = data;
    it->updateMarker(id);

    scheduleSynchronization();
}

void BreakHandler::handleAlienBreakpoint(const BreakpointResponse &response, DebuggerEngine *engine)
{
    BreakpointModelId id = findSimilarBreakpoint(response);
    if (id.isValid()) {
        if (response.id.isMinor())
            insertSubBreakpoint(id, response);
        else
            setResponse(id, response);
    } else {
        BreakpointModelId id(++currentId);
        const int row = m_storage.size();

        beginInsertRows(QModelIndex(), row, row);
        Iterator it = m_storage.insert(id, BreakpointItem());
        endInsertRows();

        it->data = response;
        it->response = response;
        it->state = BreakpointInserted;
        it->engine = engine;
        it->updateMarker(id);

        layoutChanged();
        scheduleSynchronization();
    }
}

BreakpointModelId BreakHandler::at(int n) const
{
    if (n < 0 || n >= m_storage.size())
        return BreakpointModelId();
    ConstIterator it = m_storage.constBegin();
    for ( ; --n >= 0; ++it)
        ;
    return it.key();
}

int BreakHandler::indexOf(BreakpointModelId id) const
{
    int row = 0;
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it, ++row)
        if (it.key() == id)
            return row;
    return -1;
}

void BreakHandler::insertSubBreakpoint(BreakpointModelId id,
    const BreakpointResponse &data)
{
    QTC_ASSERT(data.id.isMinor(), return);
    QTC_ASSERT(id.isMajor(), return);
    Iterator it = m_storage.find(id);

    if (it == m_storage.end()) {
        qDebug() << "FAILED: " << id.toString();
        for (ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
            it != et; ++it) {
            qDebug() << "   ID: " << it->response.id.toString();
            qDebug() << "   DATA: " << it->data.toString();
            qDebug() << "   RESP: " << it->response.toString();
        }
    }

    QTC_ASSERT(it != m_storage.end(), return);
    int minorPart = data.id.minorPart();
    int pos = -1;
    for (int i = 0; i != it->subItems.size(); ++i) {
        if (it->subItems.at(i).id.minorPart() == minorPart) {
            pos = i;
            break;
        }
    }
    if (pos == -1) {
        // This is a new sub-breakpoint.
        //qDebug() << "NEW ID" << id;
        int row = indexOf(id);
        QTC_ASSERT(row != -1, return);
        QModelIndex idx = createIndex(row, 0, id.toInternalId());
        beginInsertRows(idx, it->subItems.size(), it->subItems.size());
        it->subItems.append(data);
        endInsertRows();
    } else {
        // This modifies an existing sub-breakpoint.
        //qDebug() << "EXISTING ID" << id;
        it->subItems[pos] = data;
        layoutChanged();
    }
}

void BreakHandler::saveSessionData()
{
    saveBreakpoints();
}

void BreakHandler::loadSessionData()
{
    beginResetModel();
    m_storage.clear();
    endResetModel();
    loadBreakpoints();
}

void BreakHandler::removeSessionData()
{
    beginResetModel();
    Iterator it = m_storage.begin(), et = m_storage.end();
    for ( ; it != et; ++it)
        it->destroyMarker();
    m_storage.clear();
    endResetModel();
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

QIcon BreakHandler::icon(BreakpointModelId id) const
{
    ConstIterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), qDebug() << "NO ICON FOR ID" << id;
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

void BreakHandler::gotoLocation(BreakpointModelId id) const
{
    ConstIterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return);
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

void BreakHandler::updateFileNameFromMarker(BreakpointModelId id, const QString &fileName)
{
    Iterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return);
    it->data.fileName = fileName;
    emit layoutChanged();
}

void BreakHandler::updateLineNumberFromMarker(BreakpointModelId id, int lineNumber)
{
    Iterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return);
    // Ignore updates to the "real" line number while the debugger is
    // running, as this can be triggered by moving the breakpoint to
    // the next line that generated code.
    if (it->data.lineNumber == lineNumber)
        ; // Nothing
    else if (isEngineRunning(id))
        it->data.lineNumber += lineNumber - it->response.lineNumber;
    else
        it->data.lineNumber = lineNumber;
    it->updateMarker(id);
    emit layoutChanged();
}

void BreakHandler::changeLineNumberFromMarker(BreakpointModelId id, int lineNumber)
{
    // We need to delay this as it is called from a marker which will be destroyed.
    ExtensionSystem::InvokerBase invoker;
    invoker.addArgument(id);
    invoker.addArgument(lineNumber);
    invoker.setConnectionType(Qt::QueuedConnection);
    invoker.invoke(this, "changeLineNumberFromMarkerHelper");
    QTC_CHECK(invoker.wasSuccessful());
}

void BreakHandler::changeLineNumberFromMarkerHelper(BreakpointModelId id, int lineNumber)
{
    BreakpointParameters data = breakpointData(id);
    data.lineNumber = lineNumber;
    removeBreakpoint(id);
    appendBreakpoint(data);
}

BreakpointModelIds BreakHandler::allBreakpointIds() const
{
    BreakpointModelIds ids;
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it)
        ids.append(it.key());
    return ids;
}

BreakpointModelIds BreakHandler::unclaimedBreakpointIds() const
{
    return engineBreakpointIds(0);
}

BreakpointModelIds BreakHandler::engineBreakpointIds(DebuggerEngine *engine) const
{
    BreakpointModelIds ids;
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it)
        if (it->engine == engine)
            ids.append(it.key());
    return ids;
}

QStringList BreakHandler::engineBreakpointPaths(DebuggerEngine *engine) const
{
    QSet<QString> set;
    ConstIterator it = m_storage.constBegin(), et = m_storage.constEnd();
    for ( ; it != et; ++it) {
        if (it->engine == engine) {
            if (it->data.type == BreakpointByFileAndLine)
                set.insert(QFileInfo(it->data.fileName).dir().path());
        }
    }
    return set.toList();
}

void BreakHandler::cleanupBreakpoint(BreakpointModelId id)
{
    QTC_ASSERT(state(id) == BreakpointDead, qDebug() << state(id));
    BreakpointItem item = m_storage.take(id);
    item.destroyMarker();
    layoutChanged();
}

const BreakpointResponse &BreakHandler::response(BreakpointModelId id) const
{
    static BreakpointResponse dummy;
    ConstIterator it = m_storage.find(id);
    if (it == m_storage.end()) {
        qDebug() << "NO RESPONSE FOR " << id;
        return dummy;
    }
    return it->response;
}

bool BreakHandler::needsChange(BreakpointModelId id) const
{
    ConstIterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return false);
    return it->needsChange();
}

void BreakHandler::setResponse(BreakpointModelId id,
    const BreakpointResponse &response)
{
    Iterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return);
    it->response = response;
    it->destroyMarker();
    it->updateMarker(id);
    // Take over corrected values from response.
    if ((it->data.type == BreakpointByFileAndLine
                || it->data.type == BreakpointByFunction)
            && !response.module.isEmpty())
        it->data.module = response.module;
}

void BreakHandler::changeBreakpointData(BreakpointModelId id,
    const BreakpointParameters &data, BreakpointParts parts)
{
    Q_UNUSED(parts);
    Iterator it = m_storage.find(id);
    BREAK_ASSERT(it != m_storage.end(), return);
    if (data == it->data)
        return;
    it->data = data;
    it->destroyMarker();
    it->updateMarker(id);
    layoutChanged();
    if (it->needsChange() && it->engine && it->state != BreakpointNew) {
        setState(id, BreakpointChangeRequested);
        scheduleSynchronization();
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
    if (data.type == BreakpointByFileAndLine && data.lineNumber != response.lineNumber)
        return true;
    // FIXME: Too strict, functions may have parameter lists, or not.
    // if (data.type == BreakpointByFunction && data.functionName != response.functionName)
    //     return true;
    // if (data.type == BreakpointByAddress && data.address != response.address)
    //     return true;
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

void BreakHandler::BreakpointItem::updateMarkerIcon()
{
    if (marker) {
        marker->setIcon(icon());
        marker->updateMarker();
    }
}

void BreakHandler::BreakpointItem::updateMarker(BreakpointModelId id)
{
    QString file = markerFileName();
    int line = markerLineNumber();
    if (marker && (file != marker->fileName() || line != marker->lineNumber()))
        destroyMarker();

    if (!marker && !file.isEmpty() && line > 0) {
        marker = new BreakpointMarker(id, file, line);
        marker->init();
    }
}

QIcon BreakHandler::BreakpointItem::icon() const
{
    // FIXME: This seems to be called on each cursor blink as soon as the
    // cursor is near a line with a breakpoint marker (+/- 2 lines or so).
    if (data.isTracepoint())
        return BreakHandler::tracepointIcon();
    if (data.type == WatchpointAtAddress)
        return BreakHandler::watchpointIcon();
    if (data.type == WatchpointAtExpression)
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
        //<< "<tr><td>" << tr("ID:") << "</td><td>" << m_id << "</td></tr>"
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
            << "</td><td>" << response.id.toString() << "</td></tr>";
    }
    str << "<tr><td>" << tr("Breakpoint Type:")
        << "</td><td>" << typeToString(data.type) << "</td></tr>"
        << "<tr><td>" << tr("Marker File:")
        << "</td><td>" << QDir::toNativeSeparators(markerFileName()) << "</td></tr>"
        << "<tr><td>" << tr("Marker Line:")
        << "</td><td>" << markerLineNumber() << "</td></tr>"
        << "</table><br><hr><table>"
        << "<tr><th>" << tr("Property")
        << "</th><th>" << tr("Requested")
        << "</th><th>" << tr("Obtained") << "</th></tr>"
        << "<tr><td>" << tr("Internal Number:")
        << "</td><td>&mdash;</td><td>" << response.id.toString() << "</td></tr>";
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
            << "</td><td>"
            << "</td></tr>";
    }
    if (!data.command.isEmpty() || !response.command.isEmpty()) {
        str << "<tr><td>" << tr("Command:")
            << "</td><td>" << data.command
            << "</td><td>" << response.command
            << "</td></tr>";
    }
    if (!data.message.isEmpty() || !response.message.isEmpty()) {
        str << "<tr><td>" << tr("Message:")
            << "</td><td>" << data.message
            << "</td><td>" << response.message
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

void BreakHandler::setWatchpointAtAddress(quint64 address, unsigned size)
{
    BreakpointParameters data(WatchpointAtAddress);
    data.address = address;
    data.size = size;
    BreakpointModelId id = findWatchpoint(data);
    if (id) {
        qDebug() << "WATCHPOINT EXISTS";
        //   removeBreakpoint(index);
        return;
    }
    appendBreakpoint(data);
}

void BreakHandler::setWatchpointAtExpression(const QString &exp)
{
    BreakpointParameters data(WatchpointAtExpression);
    data.expression = exp;
    BreakpointModelId id = findWatchpoint(data);
    if (id) {
        qDebug() << "WATCHPOINT EXISTS";
        //   removeBreakpoint(index);
        return;
    }
    appendBreakpoint(data);
}

} // namespace Internal
} // namespace Debugger
