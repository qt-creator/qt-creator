/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "breakhandler.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerstringutils.h"
#include "simplifytype.h"

#include <extensionsystem/invoker.h>
#include <texteditor/textmark.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#if USE_BREAK_MODEL_TEST
#include <modeltest.h>
#endif

#include <QTimerEvent>
#include <QDir>
#include <QDebug>

#define BREAK_ASSERT(cond, action) if (cond) {} else { action; }
//#define BREAK_ASSERT(cond, action) QTC_ASSERT(cond, action)

using namespace Utils;

namespace Debugger {
namespace Internal {

//
// BreakpointMarker
//

// The red blob on the left side in the cpp editor.
class BreakpointMarker : public TextEditor::TextMark
{
public:
    BreakpointMarker(BreakpointModelId id, const QString &fileName, int lineNumber)
        : TextMark(fileName, lineNumber), m_id(id)
    {
        setIcon(breakHandler()->icon(m_id));
        setPriority(TextEditor::TextMark::NormalPriority);
        //qDebug() << "CREATE MARKER " << fileName << lineNumber;
    }

    void removedFromEditor()
    {
        breakHandler()->removeBreakpoint(m_id);
    }

    void updateLineNumber(int lineNumber)
    {
        TextMark::updateLineNumber(lineNumber);
        breakHandler()->updateLineNumberFromMarker(m_id, lineNumber);
    }

    void updateFileName(const QString &fileName)
    {
        TextMark::updateFileName(fileName);
        breakHandler()->updateFileNameFromMarker(m_id, fileName);
    }

    bool isDraggable() const { return true; }
    void dragToLine(int line) { breakHandler()->changeLineNumberFromMarker(m_id, line); }
    bool isClickable() const { return true; }
    void clicked() { breakHandler()->removeBreakpoint(m_id); }

public:
    BreakpointModelId m_id;
};

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
        case UnknownBreakpointType:
        case LastBreakpointType:
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
    auto root = new TreeItem(QStringList()
        << tr("Number") <<  tr("Function") << tr("File") << tr("Line")
        << tr("Address") << tr("Condition") << tr("Ignore") << tr("Threads"));
    setRootItem(root);
}

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
    if (Utils::HostOsInfo::fileNameCaseSensitivity() == Qt::CaseInsensitive)
        return f1.compare(f2, Qt::CaseInsensitive) == 0;
    return f1 == f2;
}

static bool isSimilarTo(const BreakpointParameters &params, const BreakpointResponse &needle)
{
    // Clear miss.
    if (needle.type != UnknownBreakpointType && params.type != UnknownBreakpointType
            && params.type != needle.type)
        return false;

    // Clear hit.
    if (params.address && params.address == needle.address)
        return true;

    // Clear hit.
    if (params == needle)
        return true;

    // At least at a position we were looking for.
    // FIXME: breaks multiple breakpoints at the same location
    if (!params.fileName.isEmpty()
            && fileNameMatch(params.fileName, needle.fileName)
            && params.lineNumber == needle.lineNumber)
        return true;

    // At least at a position we were looking for.
    // FIXME: breaks multiple breakpoints at the same location
    if (!params.fileName.isEmpty()
            && fileNameMatch(params.fileName, needle.fileName)
            && params.lineNumber == needle.lineNumber)
        return true;

    return false;
}

BreakpointModelId BreakHandler::findSimilarBreakpoint(const BreakpointResponse &needle) const
{
    // Search a breakpoint we might refer to.
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        //qDebug() << "COMPARING " << params.toString() << " WITH " << needle.toString();
        if (b->response.id.isValid() && b->response.id.majorPart() == needle.id.majorPart())
            return b->id;

        if (isSimilarTo(b->params, needle))
            return b->id;
    }
    return BreakpointModelId();
}

BreakpointModelId BreakHandler::findBreakpointByResponseId(const BreakpointResponseId &id) const
{
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->response.id.majorPart() == id.majorPart())
            return b->id;
    }
    return BreakpointModelId();
}

BreakpointModelId BreakHandler::findBreakpointByFunction(const QString &functionName) const
{
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->params.functionName == functionName)
            return b->id;
    }
    return BreakpointModelId();
}

BreakpointModelId BreakHandler::findBreakpointByAddress(quint64 address) const
{
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->params.address == address || b->params.address == address)
            return b->id;
    }
    return BreakpointModelId();
}

BreakpointModelId BreakHandler::findBreakpointByFileAndLine(const QString &fileName,
    int lineNumber, bool useMarkerPosition)
{
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->isLocatedAt(fileName, lineNumber, useMarkerPosition))
            return b->id;
    }
    return BreakpointModelId();
}

BreakHandler::BreakpointItem *BreakHandler::breakpointById(BreakpointModelId id) const
{
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->id == id)
            return b;
    }
    return 0;
}

const BreakpointParameters &BreakHandler::breakpointData(BreakpointModelId id) const
{
    static BreakpointParameters dummy;
    if (BreakpointItem *b = breakpointById(id))
        return b->params;

    BREAK_ASSERT(false, /**/);
    return dummy;
}

BreakpointModelId BreakHandler::findWatchpoint(const BreakpointParameters &params) const
{
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->params.isWatchpoint()
                && b->params.address == params.address
                && b->params.size == params.size
                && b->params.expression == params.expression
                && b->params.bitpos == params.bitpos)
            return b->id;
    }
    return BreakpointModelId();
}

void BreakHandler::saveBreakpoints()
{
    const QString one = _("1");
    QList<QVariant> list;
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        const BreakpointParameters &params = b->params;
        QMap<QString, QVariant> map;
        if (params.type != BreakpointByFileAndLine)
            map.insert(_("type"), params.type);
        if (!params.fileName.isEmpty())
            map.insert(_("filename"), params.fileName);
        if (params.lineNumber)
            map.insert(_("linenumber"), params.lineNumber);
        if (!params.functionName.isEmpty())
            map.insert(_("funcname"), params.functionName);
        if (params.address)
            map.insert(_("address"), params.address);
        if (!params.condition.isEmpty())
            map.insert(_("condition"), params.condition);
        if (params.ignoreCount)
            map.insert(_("ignorecount"), params.ignoreCount);
        if (params.threadSpec >= 0)
            map.insert(_("threadspec"), params.threadSpec);
        if (!params.enabled)
            map.insert(_("disabled"), one);
        if (params.oneShot)
            map.insert(_("oneshot"), one);
        if (params.pathUsage != BreakpointPathUsageEngineDefault)
            map.insert(_("usefullpath"), QString::number(params.pathUsage));
        if (params.tracepoint)
            map.insert(_("tracepoint"), one);
        if (!params.module.isEmpty())
            map.insert(_("module"), params.module);
        if (!params.command.isEmpty())
            map.insert(_("command"), params.command);
        if (!params.expression.isEmpty())
            map.insert(_("expression"), params.expression);
        if (!params.message.isEmpty())
            map.insert(_("message"), params.message);
        list.append(map);
    }
    setSessionValue("Breakpoints", list);
}

void BreakHandler::loadBreakpoints()
{
    QVariant value = sessionValue("Breakpoints");
    QList<QVariant> list = value.toList();
    foreach (const QVariant &var, list) {
        const QMap<QString, QVariant> map = var.toMap();
        BreakpointParameters params(BreakpointByFileAndLine);
        QVariant v = map.value(_("filename"));
        if (v.isValid())
            params.fileName = v.toString();
        v = map.value(_("linenumber"));
        if (v.isValid())
            params.lineNumber = v.toString().toInt();
        v = map.value(_("condition"));
        if (v.isValid())
            params.condition = v.toString().toLatin1();
        v = map.value(_("address"));
        if (v.isValid())
            params.address = v.toString().toULongLong();
        v = map.value(_("ignorecount"));
        if (v.isValid())
            params.ignoreCount = v.toString().toInt();
        v = map.value(_("threadspec"));
        if (v.isValid())
            params.threadSpec = v.toString().toInt();
        v = map.value(_("funcname"));
        if (v.isValid())
            params.functionName = v.toString();
        v = map.value(_("disabled"));
        if (v.isValid())
            params.enabled = !v.toInt();
        v = map.value(_("oneshot"));
        if (v.isValid())
            params.oneShot = v.toInt();
        v = map.value(_("usefullpath"));
        if (v.isValid())
            params.pathUsage = static_cast<BreakpointPathUsage>(v.toInt());
        v = map.value(_("tracepoint"));
        if (v.isValid())
            params.tracepoint = bool(v.toInt());
        v = map.value(_("type"));
        if (v.isValid() && v.toInt() != UnknownBreakpointType)
            params.type = BreakpointType(v.toInt());
        v = map.value(_("module"));
        if (v.isValid())
            params.module = v.toString();
        v = map.value(_("command"));
        if (v.isValid())
            params.command = v.toString();
        v = map.value(_("expression"));
        if (v.isValid())
            params.expression = v.toString();
        v = map.value(_("message"));
        if (v.isValid())
            params.message = v.toString();
        if (params.isValid())
            appendBreakpointInternal(params);
        else
            qWarning("Not restoring invalid breakpoint: %s", qPrintable(params.toString()));
    }
}

void BreakHandler::updateMarkers()
{
    foreach (TreeItem *n, rootItem()->children())
        static_cast<BreakpointItem *>(n)->updateMarker();
}

BreakpointModelId BreakHandler::findBreakpointByIndex(const QModelIndex &index) const
{
    TreeItem *item = itemFromIndex(index);
    if (item && item->parent() == rootItem())
        return static_cast<BreakpointItem *>(item)->id;
    return BreakpointModelId();
}

BreakpointModelIds BreakHandler::findBreakpointsByIndex(const QList<QModelIndex> &list) const
{
    QSet<BreakpointModelId> ids;
    foreach (const QModelIndex &index, list)
        ids.insert(findBreakpointByIndex(index));
    return ids.toList();
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

const QString empty(QLatin1Char('-'));

QVariant BreakHandler::BreakpointItem::data(int column, int role) const
{
    bool orig = false;
    switch (state) {
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

    switch (column) {
        case 0:
            if (role == Qt::DisplayRole)
                return id.toString();
            if (role == Qt::DecorationRole)
                return icon();
            break;
        case 1:
            if (role == Qt::DisplayRole) {
                if (!response.functionName.isEmpty())
                    return simplifyType(response.functionName);
                if (!params.functionName.isEmpty())
                    return params.functionName;
                if (params.type == BreakpointAtMain
                        || params.type == BreakpointAtThrow
                        || params.type == BreakpointAtCatch
                        || params.type == BreakpointAtFork
                        || params.type == BreakpointAtExec
                        //|| params.type == BreakpointAtVFork
                        || params.type == BreakpointAtSysCall)
                    return typeToString(params.type);
                if (params.type == WatchpointAtAddress) {
                    quint64 address = response.address ? response.address : params.address;
                    return tr("Data at 0x%1").arg(address, 0, 16);
                }
                if (params.type == WatchpointAtExpression) {
                    QString expression = !response.expression.isEmpty()
                            ? response.expression : params.expression;
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
                if (str.isEmpty() && !params.fileName.isEmpty())
                    str = params.fileName;
                if (str.isEmpty()) {
                    QString s = QFileInfo(str).fileName();
                    if (!s.isEmpty())
                        str = s;
                }
                // FIXME: better?
                //if (params.multiple && str.isEmpty() && !response.fileName.isEmpty())
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
                if (params.lineNumber > 0)
                    return params.lineNumber;
                return empty;
            }
            if (role == Qt::UserRole + 1)
                return params.lineNumber;
            break;
        case 4:
            if (role == Qt::DisplayRole) {
                const quint64 address = orig ? params.address : response.address;
                if (address)
                    return QString::fromLatin1("0x%1").arg(address, 0, 16);
                return QVariant();
            }
            break;
        case 5:
            if (role == Qt::DisplayRole)
                return orig ? params.condition : response.condition;
            if (role == Qt::ToolTipRole)
                return tr("Breakpoint will only be hit if this condition is met.");
            if (role == Qt::UserRole + 1)
                return params.condition;
            break;
        case 6:
            if (role == Qt::DisplayRole) {
                const int ignoreCount =
                    orig ? params.ignoreCount : response.ignoreCount;
                return ignoreCount ? QVariant(ignoreCount) : QVariant(QString());
            }
            if (role == Qt::ToolTipRole)
                return tr("Breakpoint will only be hit after being ignored so many times.");
            if (role == Qt::UserRole + 1)
                return params.ignoreCount;
            break;
        case 7:
            if (role == Qt::DisplayRole)
                return displayFromThreadSpec(orig ? params.threadSpec : response.threadSpec);
            if (role == Qt::ToolTipRole)
                return tr("Breakpoint will only be hit in the specified thread(s).");
            if (role == Qt::UserRole + 1)
                return displayFromThreadSpec(params.threadSpec);
            break;
    }

    if (role == Qt::ToolTipRole && boolSetting(UseToolTipsInBreakpointsView))
        return toToolTip();

    return QVariant();
}

QVariant BreakHandler::LocationItem::data(int column, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (column) {
            case 0:
                return params.id.toString();
            case 1:
                return params.functionName;
            case 4:
                if (params.address)
                    return QString::fromLatin1("0x%1").arg(params.address, 0, 16);
        }
    }
    return QVariant();
}

#define GETTER(type, getter) \
type BreakHandler::getter(BreakpointModelId id) const \
{ \
    BreakpointItem *b = breakpointById(id); \
    BREAK_ASSERT(b, qDebug() << "ID" << id << "NOT KNOWN"; return type()); \
    return b->params.getter; \
}

#define SETTER(type, getter, setter) \
void BreakHandler::setter(BreakpointModelId id, const type &value) \
{ \
    BreakpointItem *b = breakpointById(id); \
    BREAK_ASSERT(b, qDebug() << "ID" << id << "NOT KNOWN"; return); \
    if (b->params.getter == value) \
        return; \
    b->params.getter = value; \
    if (b->state != BreakpointNew) { \
        b->state = BreakpointChangeRequested; \
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
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, return false);
    return b->params.enabled;
}

void BreakHandler::setEnabled(BreakpointModelId id, bool on)
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, return);
    //qDebug() << "SET ENABLED: " << id << b->params.isEnabled() << on;
    if (b->params.enabled == on)
        return;
    b->params.enabled = on;
    b->updateMarkerIcon();
    if (b->engine) {
        b->state = BreakpointChangeRequested;
        scheduleSynchronization();
    }
}

bool BreakHandler::isWatchpoint(BreakpointModelId id) const
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, return false);
    return b->params.isWatchpoint();
}

bool BreakHandler::isTracepoint(BreakpointModelId id) const
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, return false);
    return b->params.tracepoint;
}

bool BreakHandler::isOneShot(BreakpointModelId id) const
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, return false);
    return b->params.oneShot;
}

bool BreakHandler::needsChildren(BreakpointModelId id) const
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, return false);
    return b->response.multiple && b->rowCount() == 0;
}

void BreakHandler::setTracepoint(BreakpointModelId id, bool on)
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, return);
    if (b->params.tracepoint == on)
        return;
    b->params.tracepoint = on;
    b->updateMarkerIcon();

    if (b->engine) {
        b->state = BreakpointChangeRequested;
        scheduleSynchronization();
    }
}

void BreakHandler::setMarkerFileAndLine(BreakpointModelId id,
    const QString &fileName, int lineNumber)
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, qDebug() << "MARKER_FILE_AND_LINE: " << id; return);
    if (b->response.fileName == fileName && b->response.lineNumber == lineNumber)
        return;
    b->response.fileName = fileName;
    b->response.lineNumber = lineNumber;
    b->destroyMarker();
    b->updateMarker();
    updateItem(b);
}

BreakpointState BreakHandler::state(BreakpointModelId id) const
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, qDebug() << "STATE: " << id; return BreakpointDead);
    return b->state;
}

DebuggerEngine *BreakHandler::engine(BreakpointModelId id) const
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, qDebug() << id; return 0);
    return b->engine;
}

void BreakHandler::setEngine(BreakpointModelId id, DebuggerEngine *value)
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, qDebug() << "SET ENGINE" << id; return);
    QTC_ASSERT(b->state == BreakpointNew, qDebug() << "STATE: " << b->state <<id);
    QTC_ASSERT(!b->engine, qDebug() << "NO ENGINE" << id; return);
    b->engine = value;
    b->state = BreakpointInsertRequested;
    b->response = BreakpointResponse();
    b->updateMarker();
    //scheduleSynchronization();
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
    BreakpointItem *b = breakpointById(id);
    //qDebug() << "BREAKPOINT STATE TRANSITION, ID: " << id
    //    << " FROM: " << b->state << " TO: " << state;
    BREAK_ASSERT(b, qDebug() << id; return);
    QTC_ASSERT(isAllowedTransition(b->state, state),
        qDebug() << "UNEXPECTED BREAKPOINT STATE TRANSITION"
            << b->state << state);

    if (b->state == state) {
        qDebug() << "STATE UNCHANGED: " << id << state;
        return;
    }

    b->state = state;

    // FIXME: updateMarker() should recognize the need for icon changes.
    if (state == BreakpointInserted) {
        b->destroyMarker();
        b->updateMarker();
    }
    updateItem(b);
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
    BreakpointItem *b = breakpointById(id);
    QTC_ASSERT(state(id) == BreakpointInsertProceeding, qDebug() << state(id));
    setState(id, BreakpointInserted);
    BREAK_ASSERT(b, return);
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
    BreakpointItem *b = breakpointById(id);
    removeAllSubItems(b);
    //QTC_ASSERT(state(id) == BreakpointChangeProceeding, qDebug() << state(id));
    BREAK_ASSERT(b, return);
    b->state = BreakpointNew;
    b->engine = 0;
    b->response = BreakpointResponse();
    b->destroyMarker();
    b->updateMarker();
    if (b->params.type == WatchpointAtAddress
            || b->params.type == WatchpointAtExpression
            || b->params.type == BreakpointByAddress)
        b->params.enabled = false;
    else
        b->params.address = 0;
    updateItem(b);
}

void BreakHandler::notifyBreakpointAdjusted(BreakpointModelId id,
        const BreakpointParameters &params)
{
    BreakpointItem *b = breakpointById(id);
    QTC_ASSERT(state(id) == BreakpointInserted, qDebug() << state(id));
    BREAK_ASSERT(b, return);
    b->params = params;
    //if (b->needsChange())
    //    setState(id, BreakpointChangeRequested);
}

void BreakHandler::notifyBreakpointNeedsReinsertion(BreakpointModelId id)
{
    BreakpointItem *b = breakpointById(id);
    QTC_ASSERT(state(id) == BreakpointChangeProceeding, qDebug() << state(id));
    BREAK_ASSERT(b, return);
    b->state = BreakpointInsertRequested;
}

void BreakHandler::removeAlienBreakpoint(BreakpointModelId id)
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, return);
    b->state = BreakpointDead;
    cleanupBreakpoint(id);
}

void BreakHandler::removeBreakpoint(BreakpointModelId id)
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, return);
    switch (b->state) {
    case BreakpointRemoveRequested:
        break;
    case BreakpointInserted:
    case BreakpointInsertProceeding:
        setState(id, BreakpointRemoveRequested);
        scheduleSynchronization();
        break;
    case BreakpointNew:
        b->state = BreakpointDead;
        cleanupBreakpoint(id);
        break;
    default:
        qWarning("Warning: Cannot remove breakpoint %s in state '%s'.",
               qPrintable(id.toString()), qPrintable(stateToString(b->state)));
        b->state = BreakpointRemoveRequested;
        break;
    }
}

// Ok to be not thread-safe. The order does not matter and only the gui
// produces authoritative ids.
static int currentId = 0;

void BreakHandler::appendBreakpoint(const BreakpointParameters &params)
{
    appendBreakpointInternal(params);
    scheduleSynchronization();
}

void BreakHandler::appendBreakpointInternal(const BreakpointParameters &params)
{
    if (!params.isValid()) {
        qWarning("Not adding invalid breakpoint: %s", qPrintable(params.toString()));
        return;
    }

    BreakpointItem *b = new BreakpointItem;
    b->id = BreakpointModelId(++currentId);
    b->params = params;
    b->updateMarker();
    appendItem(rootItem(), b);
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
        BreakpointItem *b = new BreakpointItem;
        b->id = BreakpointModelId(++currentId);
        b->params = response;
        b->response = response;
        b->state = BreakpointInserted;
        b->engine = engine;
        b->updateMarker();
        appendItem(rootItem(), b);
    }
}

void BreakHandler::insertSubBreakpoint(BreakpointModelId id,
    const BreakpointResponse &params)
{
    QTC_ASSERT(params.id.isMinor(), return);
    QTC_ASSERT(id.isMajor(), return);
    BreakpointItem *b = breakpointById(id);
    QTC_ASSERT(b, qDebug() << "FAILED: " << id.toString(); return);

    int minorPart = params.id.minorPart();

    foreach (TreeItem *n, b->children()) {
        LocationItem *l = static_cast<LocationItem *>(n);
        if (l->params.id.minorPart() == minorPart) {
            // This modifies an existing sub-breakpoint.
            l->params = params;
            updateItem(l);
            return;
        }
    }

    // This is a new sub-breakpoint.
    LocationItem *l = new LocationItem;
    l->params = params;
    appendItem(b, l);

    requestExpansion(indexFromItem(b));
}

void BreakHandler::saveSessionData()
{
    saveBreakpoints();
}

void BreakHandler::loadSessionData()
{
    removeAllSubItems(rootItem());
    loadBreakpoints();
}

void BreakHandler::breakByFunction(const QString &functionName)
{
    // One breakpoint per function is enough for now. This does not handle
    // combinations of multiple conditions and ignore counts, though.
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        const BreakpointParameters &params = b->params;
        if (params.functionName == functionName
                && params.condition.isEmpty()
                && params.ignoreCount == 0)
            return;
    }
    BreakpointParameters params(BreakpointByFunction);
    params.functionName = functionName;
    appendBreakpoint(params);
}

QIcon BreakHandler::icon(BreakpointModelId id) const
{
    BreakpointItem *b = breakpointById(id);
    return b ? b->icon() : pendingBreakpointIcon();
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
    Internal::synchronizeBreakpoints();
}

void BreakHandler::gotoLocation(BreakpointModelId id) const
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, return);
    DebuggerEngine *engine = currentEngine();
    if (engine) {
        if (b->params.type == BreakpointByAddress)
            engine->gotoLocation(b->params.address);
        else
            engine->gotoLocation(Location(b->markerFileName(), b->markerLineNumber(), false));
    }
}

void BreakHandler::updateFileNameFromMarker(BreakpointModelId id, const QString &fileName)
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, return);
    b->params.fileName = fileName;
    updateItem(b);
}

void BreakHandler::updateLineNumberFromMarker(BreakpointModelId id, int lineNumber)
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, return);
    // Ignore updates to the "real" line number while the debugger is
    // running, as this can be triggered by moving the breakpoint to
    // the next line that generated code.
    if (b->params.lineNumber == lineNumber)
        ; // Nothing
    else if (isEngineRunning(id))
        b->params.lineNumber += lineNumber - b->response.lineNumber;
    else
        b->params.lineNumber = lineNumber;
    b->updateMarker();
    updateItem(b);
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
    BreakpointParameters params = breakpointData(id);
    params.lineNumber = lineNumber;
    removeBreakpoint(id);
    appendBreakpoint(params);
}

BreakpointModelIds BreakHandler::allBreakpointIds() const
{
    BreakpointModelIds ids;
    foreach (TreeItem *n, rootItem()->children())
        ids.append(static_cast<BreakpointItem *>(n)->id);
    return ids;
}

BreakpointModelIds BreakHandler::unclaimedBreakpointIds() const
{
    return engineBreakpointIds(0);
}

BreakpointModelIds BreakHandler::engineBreakpointIds(DebuggerEngine *engine) const
{
    BreakpointModelIds ids;
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->engine == engine)
            ids.append(b->id);
    }
    return ids;
}

QStringList BreakHandler::engineBreakpointPaths(DebuggerEngine *engine) const
{
    QSet<QString> set;
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->engine == engine) {
            if (b->params.type == BreakpointByFileAndLine)
                set.insert(QFileInfo(b->params.fileName).dir().path());
        }
    }
    return set.toList();
}

void BreakHandler::cleanupBreakpoint(BreakpointModelId id)
{
    QTC_ASSERT(state(id) == BreakpointDead, qDebug() << state(id));
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, return);
    b->destroyMarker();
    removeItem(b);
    delete b;
}

const BreakpointResponse &BreakHandler::response(BreakpointModelId id) const
{
    static BreakpointResponse dummy;
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, qDebug() << "NO RESPONSE FOR " << id; return dummy);
    return b->response;
}

bool BreakHandler::needsChange(BreakpointModelId id) const
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, return false);
    return b->needsChange();
}

void BreakHandler::setResponse(BreakpointModelId id,
    const BreakpointResponse &response)
{
    BreakpointItem *b = breakpointById(id);
    BREAK_ASSERT(b, return);
    b->response = response;
    b->destroyMarker();
    b->updateMarker();
    // Take over corrected values from response.
    if ((b->params.type == BreakpointByFileAndLine
                || b->params.type == BreakpointByFunction)
            && !response.module.isEmpty())
        b->params.module = response.module;
}

void BreakHandler::changeBreakpointData(BreakpointModelId id,
    const BreakpointParameters &params, BreakpointParts parts)
{
    BreakpointItem *b = breakpointById(id);
    Q_UNUSED(parts);
    BREAK_ASSERT(b, return);
    if (params == b->params)
        return;
    b->params = params;
    b->destroyMarker();
    b->updateMarker();
    updateItem(b);
    if (b->needsChange() && b->engine && b->state != BreakpointNew) {
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

BreakHandler::BreakpointItem::~BreakpointItem()
{
    delete marker;
}

void BreakHandler::BreakpointItem::destroyMarker()
{
    BreakpointMarker *m = marker;
    marker = 0;
    delete m;
}

QString BreakHandler::BreakpointItem::markerFileName() const
{
    // Some heuristics to find a "good" file name.
    if (!params.fileName.isEmpty()) {
        QFileInfo fi(params.fileName);
        if (fi.exists())
            return fi.absoluteFilePath();
    }
    if (!response.fileName.isEmpty()) {
        QFileInfo fi(response.fileName);
        if (fi.exists())
            return fi.absoluteFilePath();
    }
    if (response.fileName.endsWith(params.fileName))
        return response.fileName;
    if (params.fileName.endsWith(response.fileName))
        return params.fileName;
    return response.fileName.size() > params.fileName.size()
        ? response.fileName : params.fileName;
}

int BreakHandler::BreakpointItem::markerLineNumber() const
{
    return response.lineNumber ? response.lineNumber : params.lineNumber;
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
    if (!params.conditionsMatch(response.condition))
        return true;
    if (params.ignoreCount != response.ignoreCount)
        return true;
    if (params.enabled != response.enabled)
        return true;
    if (params.threadSpec != response.threadSpec)
        return true;
    if (params.command != response.command)
        return true;
    if (params.type == BreakpointByFileAndLine && params.lineNumber != response.lineNumber)
        return true;
    // FIXME: Too strict, functions may have parameter lists, or not.
    // if (params.type == BreakpointByFunction && params.functionName != response.functionName)
    //     return true;
    // if (params.type == BreakpointByAddress && params.address != response.address)
    //     return true;
    return false;
}

bool BreakHandler::BreakpointItem::isLocatedAt
    (const QString &fileName, int lineNumber, bool useMarkerPosition) const
{
    int line = useMarkerPosition ? response.lineNumber : params.lineNumber;
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

void BreakHandler::BreakpointItem::updateMarker()
{
    QString file = markerFileName();
    int line = markerLineNumber();
    if (marker && (file != marker->fileName() || line != marker->lineNumber()))
        destroyMarker();

    if (!marker && !file.isEmpty() && line > 0)
        marker = new BreakpointMarker(id, file, line);
}

QIcon BreakHandler::BreakpointItem::icon() const
{
    // FIXME: This seems to be called on each cursor blink as soon as the
    // cursor is near a line with a breakpoint marker (+/- 2 lines or so).
    if (params.isTracepoint())
        return BreakHandler::tracepointIcon();
    if (params.type == WatchpointAtAddress)
        return BreakHandler::watchpointIcon();
    if (params.type == WatchpointAtExpression)
        return BreakHandler::watchpointIcon();
    if (!params.enabled)
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
        << "</td><td>" << (params.enabled ? tr("Enabled") : tr("Disabled"));
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
        << "</td><td>" << typeToString(params.type) << "</td></tr>"
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
    if (params.type == BreakpointByFunction) {
        str << "<tr><td>" << tr("Function Name:")
        << "</td><td>" << params.functionName
        << "</td><td>" << response.functionName
        << "</td></tr>";
    }
    if (params.type == BreakpointByFileAndLine) {
    str << "<tr><td>" << tr("File Name:")
        << "</td><td>" << QDir::toNativeSeparators(params.fileName)
        << "</td><td>" << QDir::toNativeSeparators(response.fileName)
        << "</td></tr>"
        << "<tr><td>" << tr("Line Number:")
        << "</td><td>" << params.lineNumber
        << "</td><td>" << response.lineNumber << "</td></tr>"
        << "<tr><td>" << tr("Corrected Line Number:")
        << "</td><td>-"
        << "</td><td>" << response.correctedLineNumber << "</td></tr>";
    }
    if (params.type == BreakpointByFunction || params.type == BreakpointByFileAndLine) {
        str << "<tr><td>" << tr("Module:")
            << "</td><td>" << params.module
            << "</td><td>" << response.module
            << "</td></tr>";
    }
    str << "<tr><td>" << tr("Breakpoint Address:")
        << "</td><td>";
    formatAddress(str, params.address);
    str << "</td><td>";
    formatAddress(str, response.address);
    str << "</td></tr>";
    if (response.multiple) {
        str << "<tr><td>" << tr("Multiple Addresses:")
            << "</td><td>"
            << "</td></tr>";
    }
    if (!params.command.isEmpty() || !response.command.isEmpty()) {
        str << "<tr><td>" << tr("Command:")
            << "</td><td>" << params.command
            << "</td><td>" << response.command
            << "</td></tr>";
    }
    if (!params.message.isEmpty() || !response.message.isEmpty()) {
        str << "<tr><td>" << tr("Message:")
            << "</td><td>" << params.message
            << "</td><td>" << response.message
            << "</td></tr>";
    }
    if (!params.condition.isEmpty() || !response.condition.isEmpty()) {
        str << "<tr><td>" << tr("Condition:")
            << "</td><td>" << params.condition
            << "</td><td>" << response.condition
            << "</td></tr>";
    }
    if (params.ignoreCount || response.ignoreCount) {
        str << "<tr><td>" << tr("Ignore Count:") << "</td><td>";
        if (params.ignoreCount)
            str << params.ignoreCount;
        str << "</td><td>";
        if (response.ignoreCount)
            str << response.ignoreCount;
        str << "</td></tr>";
    }
    if (params.threadSpec >= 0 || response.threadSpec >= 0) {
        str << "<tr><td>" << tr("Thread Specification:")
            << "</td><td>";
        if (params.threadSpec >= 0)
            str << params.threadSpec;
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
    BreakpointParameters params(WatchpointAtAddress);
    params.address = address;
    params.size = size;
    BreakpointModelId id = findWatchpoint(params);
    if (id) {
        qDebug() << "WATCHPOINT EXISTS";
        //   removeBreakpoint(index);
        return;
    }
    appendBreakpoint(params);
}

void BreakHandler::setWatchpointAtExpression(const QString &exp)
{
    BreakpointParameters params(WatchpointAtExpression);
    params.expression = exp;
    BreakpointModelId id = findWatchpoint(params);
    if (id) {
        qDebug() << "WATCHPOINT EXISTS";
        //   removeBreakpoint(index);
        return;
    }
    appendBreakpoint(params);
}

} // namespace Internal
} // namespace Debugger
