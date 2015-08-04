/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "breakhandler.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "debuggerstringutils.h"
#include "simplifytype.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugin.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>

#include <extensionsystem/invoker.h>
#include <texteditor/textmark.h>
#include <texteditor/texteditor.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/fileutils.h>
#include <utils/theme/theme.h>

#if USE_BREAK_MODEL_TEST
#include <modeltest.h>
#endif

#include <QTimerEvent>
#include <QDir>
#include <QDebug>

using namespace Core;
using namespace Utils;

namespace Debugger {
namespace Internal {

struct LocationItem : public TreeItem
{
    QVariant data(int column, int role) const
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

    BreakpointResponse params;
};

class BreakpointMarker;

class BreakpointItem : public QObject, public TreeItem
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::BreakHandler)

public:
    ~BreakpointItem();

    QVariant data(int column, int role) const;

    QIcon icon() const;

    void removeBreakpoint();
    void updateLineNumberFromMarker(int lineNumber);
    void updateFileNameFromMarker(const QString &fileName);
    void changeLineNumberFromMarker(int lineNumber);
    bool isLocatedAt(const QString &fileName, int lineNumber, bool useMarkerPosition) const;

    bool needsChildren() const;

    void setMarkerFileAndLine(const QString &fileName, int lineNumber);

    void insertSubBreakpoint(const BreakpointResponse &params);
    QString markerFileName() const;
    int markerLineNumber() const;

    bool needsChange() const;
private:
    friend class BreakHandler;
    friend class Breakpoint;
    BreakpointItem(BreakHandler *handler);

    void destroyMarker();
    void updateMarker();
    void updateMarkerIcon();
    void scheduleSynchronization();

    QString toToolTip() const;
    void setState(BreakpointState state);
    void deleteThis();
    bool isEngineRunning() const;

    BreakHandler * const m_handler;
    const BreakpointModelId m_id;
    BreakpointParameters m_params;
    BreakpointState m_state;   // Current state of breakpoint.
    DebuggerEngine *m_engine;  // Engine currently handling the breakpoint.
    BreakpointResponse m_response;
    BreakpointMarker *m_marker;
};

//
// BreakpointMarker
//

// The red blob on the left side in the cpp editor.
class BreakpointMarker : public TextEditor::TextMark
{
public:
    BreakpointMarker(BreakpointItem *b, const QString &fileName, int lineNumber)
        : TextMark(fileName, lineNumber, Constants::TEXT_MARK_CATEGORY_BREAKPOINT), m_bp(b)
    {
        setIcon(b->icon());
        setPriority(TextEditor::TextMark::NormalPriority);
    }

    void removedFromEditor()
    {
        if (m_bp)
            m_bp->removeBreakpoint();
    }

    void updateLineNumber(int lineNumber)
    {
        TextMark::updateLineNumber(lineNumber);
        m_bp->updateLineNumberFromMarker(lineNumber);
    }

    void updateFileName(const QString &fileName)
    {
        TextMark::updateFileName(fileName);
        m_bp->updateFileNameFromMarker(fileName);
    }

    bool isDraggable() const { return true; }
    void dragToLine(int line) { m_bp->changeLineNumberFromMarker(line); }
    bool isClickable() const { return true; }
    void clicked() { m_bp->removeBreakpoint(); }

public:
    BreakpointItem *m_bp;
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
    qRegisterMetaType<BreakpointModelId>();
    TextEditor::TextMark::setCategoryColor(Constants::TEXT_MARK_CATEGORY_BREAKPOINT,
                                           Utils::Theme::Debugger_Breakpoint_TextMarkColor);

#if USE_BREAK_MODEL_TEST
    new ModelTest(this, 0);
#endif
    setHeader(QStringList()
        << tr("Number") <<  tr("Function") << tr("File") << tr("Line")
        << tr("Address") << tr("Condition") << tr("Ignore") << tr("Threads"));
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
    if (HostOsInfo::fileNameCaseSensitivity() == Qt::CaseInsensitive)
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

Breakpoint BreakHandler::findSimilarBreakpoint(const BreakpointResponse &needle) const
{
    // Search a breakpoint we might refer to.
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        //qDebug() << "COMPARING " << params.toString() << " WITH " << needle.toString();
        if (b->m_response.id.isValid() && b->m_response.id.majorPart() == needle.id.majorPart())
            return Breakpoint(b);

        if (isSimilarTo(b->m_params, needle))
            return Breakpoint(b);
    }
    return Breakpoint();
}

Breakpoint BreakHandler::findBreakpointByResponseId(const BreakpointResponseId &id) const
{
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->m_response.id.majorPart() == id.majorPart())
            return Breakpoint(b);
    }
    return Breakpoint();
}

Breakpoint BreakHandler::findBreakpointByFunction(const QString &functionName) const
{
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->m_params.functionName == functionName)
            return Breakpoint(b);
    }
    return Breakpoint();
}

Breakpoint BreakHandler::findBreakpointByAddress(quint64 address) const
{
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->m_params.address == address || b->m_params.address == address)
            return Breakpoint(b);
    }
    return Breakpoint();
}

Breakpoint BreakHandler::findBreakpointByFileAndLine(const QString &fileName,
    int lineNumber, bool useMarkerPosition)
{
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->isLocatedAt(fileName, lineNumber, useMarkerPosition))
            return Breakpoint(b);
    }
    return Breakpoint();
}

Breakpoint BreakHandler::breakpointById(BreakpointModelId id) const
{
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->m_id == id)
            return Breakpoint(b);
    }
    return Breakpoint();
}

void BreakHandler::deletionHelper(BreakpointModelId id)
{
    Breakpoint b = breakpointById(id);
    QTC_ASSERT(b, return);
    delete takeItem(b.b);
}

Breakpoint BreakHandler::findWatchpoint(const BreakpointParameters &params) const
{
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->m_params.isWatchpoint()
                && b->m_params.address == params.address
                && b->m_params.size == params.size
                && b->m_params.expression == params.expression
                && b->m_params.bitpos == params.bitpos)
            return Breakpoint(b);
    }
    return Breakpoint();
}

void BreakHandler::saveBreakpoints()
{
    const QString one = _("1");
    QList<QVariant> list;
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        const BreakpointParameters &params = b->m_params;
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

Breakpoint BreakHandler::findBreakpointByIndex(const QModelIndex &index) const
{
    TreeItem *item = itemForIndex(index);
    return Breakpoint(item && item->parent() == rootItem() ? static_cast<BreakpointItem *>(item) : 0);
}

Breakpoints BreakHandler::findBreakpointsByIndex(const QList<QModelIndex> &list) const
{
    QSet<Breakpoint> ids;
    foreach (const QModelIndex &index, list) {
        if (Breakpoint b = findBreakpointByIndex(index))
            ids.insert(Breakpoint(b));
    }
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

QVariant BreakpointItem::data(int column, int role) const
{
    bool orig = false;
    switch (m_state) {
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

    if (role == Qt::ForegroundRole) {
        static const QVariant gray(QColor(140, 140, 140));
        switch (m_state) {
            case BreakpointInsertRequested:
            case BreakpointInsertProceeding:
            case BreakpointChangeRequested:
            case BreakpointChangeProceeding:
            case BreakpointRemoveRequested:
            case BreakpointRemoveProceeding:
                return gray;
            case BreakpointInserted:
            case BreakpointNew:
            case BreakpointDead:
                break;
        };
    }

    switch (column) {
        case 0:
            if (role == Qt::DisplayRole)
                return m_id.toString();
            if (role == Qt::DecorationRole)
                return icon();
            break;
        case 1:
            if (role == Qt::DisplayRole) {
                if (!m_response.functionName.isEmpty())
                    return simplifyType(m_response.functionName);
                if (!m_params.functionName.isEmpty())
                    return m_params.functionName;
                if (m_params.type == BreakpointAtMain
                        || m_params.type == BreakpointAtThrow
                        || m_params.type == BreakpointAtCatch
                        || m_params.type == BreakpointAtFork
                        || m_params.type == BreakpointAtExec
                        //|| m_params.type == BreakpointAtVFork
                        || m_params.type == BreakpointAtSysCall)
                    return typeToString(m_params.type);
                if (m_params.type == WatchpointAtAddress) {
                    quint64 address = m_response.address ? m_response.address : m_params.address;
                    return BreakHandler::tr("Data at 0x%1").arg(address, 0, 16);
                }
                if (m_params.type == WatchpointAtExpression) {
                    QString expression = !m_response.expression.isEmpty()
                            ? m_response.expression : m_params.expression;
                    return BreakHandler::tr("Data at %1").arg(expression);
                }
                return empty;
            }
            break;
        case 2:
            if (role == Qt::DisplayRole) {
                QString str;
                if (!m_response.fileName.isEmpty())
                    str = m_response.fileName;
                if (str.isEmpty() && !m_params.fileName.isEmpty())
                    str = m_params.fileName;
                if (str.isEmpty()) {
                    QString s = FileName::fromString(str).fileName();
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
                if (m_response.lineNumber > 0)
                    return m_response.lineNumber;
                if (m_params.lineNumber > 0)
                    return m_params.lineNumber;
                return empty;
            }
            if (role == Qt::UserRole + 1)
                return m_params.lineNumber;
            break;
        case 4:
            if (role == Qt::DisplayRole) {
                const quint64 address = orig ? m_params.address : m_response.address;
                if (address)
                    return QString::fromLatin1("0x%1").arg(address, 0, 16);
                return QVariant();
            }
            break;
        case 5:
            if (role == Qt::DisplayRole)
                return orig ? m_params.condition : m_response.condition;
            if (role == Qt::ToolTipRole)
                return BreakHandler::tr("Breakpoint will only be hit if this condition is met.");
            if (role == Qt::UserRole + 1)
                return m_params.condition;
            break;
        case 6:
            if (role == Qt::DisplayRole) {
                const int ignoreCount =
                    orig ? m_params.ignoreCount : m_response.ignoreCount;
                return ignoreCount ? QVariant(ignoreCount) : QVariant(QString());
            }
            if (role == Qt::ToolTipRole)
                return BreakHandler::tr("Breakpoint will only be hit after being ignored so many times.");
            if (role == Qt::UserRole + 1)
                return m_params.ignoreCount;
            break;
        case 7:
            if (role == Qt::DisplayRole)
                return BreakHandler::displayFromThreadSpec(orig ? m_params.threadSpec : m_response.threadSpec);
            if (role == Qt::ToolTipRole)
                return BreakHandler::tr("Breakpoint will only be hit in the specified thread(s).");
            if (role == Qt::UserRole + 1)
                return BreakHandler::displayFromThreadSpec(m_params.threadSpec);
            break;
    }

    if (role == Qt::ToolTipRole && boolSetting(UseToolTipsInBreakpointsView))
        return toToolTip();

    return QVariant();
}

#define PROPERTY(type, getter, setter) \
\
type Breakpoint::getter() const \
{ \
    return parameters().getter; \
} \
\
void Breakpoint::setter(const type &value) \
{ \
    QTC_ASSERT(b, return); \
    if (b->m_params.getter == value) \
        return; \
    b->m_params.getter = value; \
    if (b->m_state != BreakpointNew) { \
        b->m_state = BreakpointChangeRequested; \
        b->scheduleSynchronization(); \
    } \
}

PROPERTY(BreakpointPathUsage, pathUsage, setPathUsage)
PROPERTY(QString, fileName, setFileName)
PROPERTY(QString, functionName, setFunctionName)
PROPERTY(BreakpointType, type, setType)
PROPERTY(int, threadSpec, setThreadSpec)
PROPERTY(QByteArray, condition, setCondition)
PROPERTY(quint64, address, setAddress)
PROPERTY(QString, expression, setExpression)
PROPERTY(QString, message, setMessage)
PROPERTY(int, ignoreCount, setIgnoreCount)

void BreakpointItem::scheduleSynchronization()
{
    m_handler->scheduleSynchronization();
}

const BreakpointParameters &Breakpoint::parameters() const
{
    static BreakpointParameters p;
    QTC_ASSERT(b, return p);
    return b->m_params;
}

void Breakpoint::addToCommand(DebuggerCommand *cmd) const
{
    cmd->arg("modelid", id().toByteArray());
    cmd->arg("type", type());
    cmd->arg("ignorecount", ignoreCount());
    cmd->arg("condition", condition().toHex());
    cmd->arg("function", functionName().toUtf8());
    cmd->arg("oneshot", isOneShot());
    cmd->arg("enabled", isEnabled());
    cmd->arg("fileName", fileName().toUtf8());
    cmd->arg("lineNumber", lineNumber());
    cmd->arg("address", address());
    cmd->arg("expression", expression());
}

BreakpointState Breakpoint::state() const
{
    QTC_ASSERT(b, return BreakpointState());
    return b->m_state;
}

int Breakpoint::lineNumber() const { return parameters().lineNumber; }

bool Breakpoint::isEnabled() const { return parameters().enabled; }

bool Breakpoint::isWatchpoint() const { return parameters().isWatchpoint(); }

bool Breakpoint::isTracepoint() const { return parameters().isTracepoint(); }

QIcon Breakpoint::icon() const { return b ? b->icon() : QIcon(); }

DebuggerEngine *Breakpoint::engine() const
{
    return b ? b->m_engine : 0;
}

const BreakpointResponse &Breakpoint::response() const
{
    static BreakpointResponse r;
    return b ? b->m_response : r;
}

bool Breakpoint::isOneShot() const { return parameters().oneShot; }

void Breakpoint::removeAlienBreakpoint()
{
    b->m_state = BreakpointRemoveProceeding;
    b->deleteThis();
}

void Breakpoint::removeBreakpoint() const
{
    b->removeBreakpoint();
}

Breakpoint::Breakpoint(BreakpointItem *b)
    : b(b)
{}

void Breakpoint::setEnabled(bool on) const
{
    QTC_ASSERT(b, return);
    if (b->m_params.enabled == on)
        return;
    b->m_params.enabled = on;
    b->updateMarkerIcon();
    if (b->m_engine) {
        b->m_state = BreakpointChangeRequested;
        b->scheduleSynchronization();
    }
}

void Breakpoint::setMarkerFileAndLine(const QString &fileName, int lineNumber)
{
    if (b)
        b->setMarkerFileAndLine(fileName, lineNumber);
}

bool BreakpointItem::needsChildren() const
{
    return m_response.multiple && rowCount() == 0;
}

void Breakpoint::setTracepoint(bool on)
{
    if (b->m_params.tracepoint == on)
        return;
    b->m_params.tracepoint = on;
    b->updateMarkerIcon();

    if (b->m_engine) {
        b->m_state = BreakpointChangeRequested;
        b->scheduleSynchronization();
    }
}

void BreakpointItem::setMarkerFileAndLine(const QString &fileName, int lineNumber)
{
    if (m_response.fileName == fileName && m_response.lineNumber == lineNumber)
        return;
    m_response.fileName = fileName;
    m_response.lineNumber = lineNumber;
    destroyMarker();
    updateMarker();
    update();
}

void Breakpoint::setEngine(DebuggerEngine *value)
{
    QTC_ASSERT(b->m_state == BreakpointNew, qDebug() << "STATE: " << b->m_state << b->m_id);
    QTC_ASSERT(!b->m_engine, qDebug() << "NO ENGINE" << b->m_id; return);
    b->m_engine = value;
    b->m_state = BreakpointInsertRequested;
    b->m_response = BreakpointResponse();
    b->updateMarker();
    //b->scheduleSynchronization();
}

static bool isAllowedTransition(BreakpointState from, BreakpointState to)
{
    switch (from) {
    case BreakpointNew:
        return to == BreakpointInsertRequested
            || to == BreakpointDead;
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

bool BreakpointItem::isEngineRunning() const
{
    if (!m_engine)
        return false;
    const DebuggerState state = m_engine->state();
    return state != DebuggerFinished && state != DebuggerNotReady;
}

void BreakpointItem::setState(BreakpointState state)
{
    //qDebug() << "BREAKPOINT STATE TRANSITION, ID: " << m_id
    //    << " FROM: " << state << " TO: " << state;
    if (!isAllowedTransition(m_state, state)) {
        qDebug() << "UNEXPECTED BREAKPOINT STATE TRANSITION" << m_state << state;
        QTC_CHECK(false);
    }

    if (m_state == state) {
        qDebug() << "STATE UNCHANGED: " << m_id << m_state;
        return;
    }

    m_state = state;

    // FIXME: updateMarker() should recognize the need for icon changes.
    if (state == BreakpointInserted) {
        destroyMarker();
        updateMarker();
    }
    update();
}

void BreakpointItem::deleteThis()
{
    setState(BreakpointDead);
    destroyMarker();

    // This is called from b directly. So delay deletion of b.
    ExtensionSystem::InvokerBase invoker;
    invoker.addArgument(m_id);
    invoker.setConnectionType(Qt::QueuedConnection);
    invoker.invoke(m_handler, "deletionHelper");
    QTC_CHECK(invoker.wasSuccessful());
}

void Breakpoint::gotoState(BreakpointState target, BreakpointState assumedCurrent)
{
    QTC_ASSERT(b, return);
    QTC_ASSERT(b->m_state == assumedCurrent, qDebug() << b->m_state);
    b->setState(target);
}

void Breakpoint::notifyBreakpointChangeAfterInsertNeeded()
{
    gotoState(BreakpointChangeRequested, BreakpointInsertProceeding);
}

void Breakpoint::notifyBreakpointInsertProceeding()
{
    gotoState(BreakpointInsertProceeding, BreakpointInsertRequested);
}

void Breakpoint::notifyBreakpointInsertOk()
{
    gotoState(BreakpointInserted, BreakpointInsertProceeding);
}

void Breakpoint::notifyBreakpointInsertFailed()
{
    gotoState(BreakpointDead, BreakpointInsertProceeding);
}

void Breakpoint::notifyBreakpointRemoveProceeding()
{
    gotoState(BreakpointRemoveProceeding, BreakpointRemoveRequested);
}

void Breakpoint::notifyBreakpointRemoveOk()
{
    QTC_ASSERT(b, return);
    QTC_ASSERT(b->m_state == BreakpointRemoveProceeding, qDebug() << b->m_state);
    b->deleteThis();
}

void Breakpoint::notifyBreakpointRemoveFailed()
{
    QTC_ASSERT(b, return);
    QTC_ASSERT(b->m_state == BreakpointRemoveProceeding, qDebug() << b->m_state);
    b->deleteThis();
}

void Breakpoint::notifyBreakpointChangeProceeding()
{
    gotoState(BreakpointChangeProceeding, BreakpointChangeRequested);
}

void Breakpoint::notifyBreakpointChangeOk()
{
    gotoState(BreakpointInserted, BreakpointChangeProceeding);
}

void Breakpoint::notifyBreakpointChangeFailed()
{
    gotoState(BreakpointDead, BreakpointChangeProceeding);
}

void Breakpoint::notifyBreakpointReleased()
{
    QTC_ASSERT(b, return);
    b->removeChildren();
    //QTC_ASSERT(b->m_state == BreakpointChangeProceeding, qDebug() << b->m_state);
    b->m_state = BreakpointNew;
    b->m_engine = 0;
    b->m_response = BreakpointResponse();
    b->destroyMarker();
    b->updateMarker();
    if (b->m_params.type == WatchpointAtAddress
            || b->m_params.type == WatchpointAtExpression
            || b->m_params.type == BreakpointByAddress)
        b->m_params.enabled = false;
    else
        b->m_params.address = 0;
    b->update();
}

void Breakpoint::notifyBreakpointAdjusted(const BreakpointParameters &params)
{
    QTC_ASSERT(b, return);
    QTC_ASSERT(b->m_state == BreakpointInserted, qDebug() << b->m_state);
    b->m_params = params;
    //if (b->needsChange())
    //    b->setState(BreakpointChangeRequested);
}

void Breakpoint::notifyBreakpointNeedsReinsertion()
{
    QTC_ASSERT(b, return);
    QTC_ASSERT(b->m_state == BreakpointChangeProceeding, qDebug() << b->m_state);
    b->m_state = BreakpointInsertRequested;
}

void BreakpointItem::removeBreakpoint()
{
    switch (m_state) {
    case BreakpointRemoveRequested:
        break;
    case BreakpointInserted:
    case BreakpointInsertProceeding:
        setState(BreakpointRemoveRequested);
        scheduleSynchronization();
        break;
    case BreakpointNew:
        deleteThis();
        break;
    default:
        qWarning("Warning: Cannot remove breakpoint %s in state '%s'.",
               qPrintable(m_id.toString()), qPrintable(stateToString(m_state)));
        m_state = BreakpointRemoveRequested;
        break;
    }
}

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

    BreakpointItem *b = new BreakpointItem(this);
    b->m_params = params;
    b->updateMarker();
    rootItem()->appendChild(b);
}

void BreakHandler::handleAlienBreakpoint(const BreakpointResponse &response, DebuggerEngine *engine)
{
    Breakpoint b = findSimilarBreakpoint(response);
    if (b) {
        if (response.id.isMinor())
            b.insertSubBreakpoint(response);
        else
            b.setResponse(response);
    } else {
        auto b = new BreakpointItem(this);
        b->m_params = response;
        b->m_response = response;
        b->m_state = BreakpointInserted;
        b->m_engine = engine;
        b->updateMarker();
        rootItem()->appendChild(b);
    }
}

void Breakpoint::insertSubBreakpoint(const BreakpointResponse &params)
{
    QTC_ASSERT(b, return);
    b->insertSubBreakpoint(params);
}

void BreakpointItem::insertSubBreakpoint(const BreakpointResponse &params)
{
    QTC_ASSERT(params.id.isMinor(), return);

    int minorPart = params.id.minorPart();

    const QVector<TreeItem *> &children = TreeItem::children();
    foreach (TreeItem *n, children) {
        LocationItem *l = static_cast<LocationItem *>(n);
        if (l->params.id.minorPart() == minorPart) {
            // This modifies an existing sub-breakpoint.
            l->params = params;
            l->update();
            return;
        }
    }

    // This is a new sub-breakpoint.
    LocationItem *l = new LocationItem;
    l->params = params;
    appendChild(l);
    expand();
}

void BreakHandler::saveSessionData()
{
    saveBreakpoints();
}

void BreakHandler::loadSessionData()
{
    clear();
    loadBreakpoints();
}

void BreakHandler::breakByFunction(const QString &functionName)
{
    // One breakpoint per function is enough for now. This does not handle
    // combinations of multiple conditions and ignore counts, though.
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        const BreakpointParameters &params = b->m_params;
        if (params.functionName == functionName
                && params.condition.isEmpty()
                && params.ignoreCount == 0)
            return;
    }
    BreakpointParameters params(BreakpointByFunction);
    params.functionName = functionName;
    appendBreakpoint(params);
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

void Breakpoint::gotoLocation() const
{
    if (DebuggerEngine *engine = currentEngine()) {
        if (b->m_params.type == BreakpointByAddress) {
            engine->gotoLocation(b->m_params.address);
        } else {
            // Don't use gotoLocation unconditionally as this ends up in
            // disassembly if OperateByInstruction is on. But fallback
            // to disassembly if we can't open the file.
            const QString file = QDir::cleanPath(b->markerFileName());
            if (IEditor *editor = EditorManager::openEditor(file))
                editor->gotoLine(b->markerLineNumber(), 0);
            else
                engine->openDisassemblerView(Location(b->m_response.address));
        }
    }
}

void BreakpointItem::updateFileNameFromMarker(const QString &fileName)
{
    m_params.fileName = fileName;
    update();
}

void BreakpointItem::updateLineNumberFromMarker(int lineNumber)
{
    // Ignore updates to the "real" line number while the debugger is
    // running, as this can be triggered by moving the breakpoint to
    // the next line that generated code.
    if (m_params.lineNumber == lineNumber)
        ; // Nothing
    else if (isEngineRunning())
        m_params.lineNumber += lineNumber - m_response.lineNumber;
    else
        m_params.lineNumber = lineNumber;
    updateMarker();
    update();
}

void BreakpointItem::changeLineNumberFromMarker(int lineNumber)
{
    m_params.lineNumber = lineNumber;

    // We need to delay this as it is called from a marker which will be destroyed.
    ExtensionSystem::InvokerBase invoker;
    invoker.addArgument(m_id);
    invoker.setConnectionType(Qt::QueuedConnection);
    invoker.invoke(m_handler, "changeLineNumberFromMarkerHelper");
    QTC_CHECK(invoker.wasSuccessful());
}

void BreakHandler::changeLineNumberFromMarkerHelper(BreakpointModelId id)
{
    Breakpoint b = breakpointById(id);
    QTC_ASSERT(b, return);
    BreakpointParameters params = b.parameters();
    delete takeItem(b.b);
    appendBreakpoint(params);
}

Breakpoints BreakHandler::allBreakpoints() const
{
    Breakpoints items;
    foreach (TreeItem *n, rootItem()->children())
        items.append(Breakpoint(static_cast<BreakpointItem *>(n)));
    return items;
}

Breakpoints BreakHandler::unclaimedBreakpoints() const
{
    return engineBreakpoints(0);
}

Breakpoints BreakHandler::engineBreakpoints(DebuggerEngine *engine) const
{
    Breakpoints items;
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->m_engine == engine)
            items.append(Breakpoint(b));
    }
    return items;
}

QStringList BreakHandler::engineBreakpointPaths(DebuggerEngine *engine) const
{
    QSet<QString> set;
    foreach (TreeItem *n, rootItem()->children()) {
        BreakpointItem *b = static_cast<BreakpointItem *>(n);
        if (b->m_engine == engine) {
            if (b->m_params.type == BreakpointByFileAndLine)
                set.insert(QFileInfo(b->m_params.fileName).dir().path());
        }
    }
    return set.toList();
}

void Breakpoint::setResponse(const BreakpointResponse &response)
{
    QTC_ASSERT(b, return);
    b->m_response = response;
    b->destroyMarker();
    b->updateMarker();
    // Take over corrected values from response.
    if ((b->m_params.type == BreakpointByFileAndLine
                || b->m_params.type == BreakpointByFunction)
            && !response.module.isEmpty())
        b->m_params.module = response.module;
}

bool Internal::Breakpoint::needsChange() const
{
    return b && b->needsChange();
}

void Breakpoint::changeBreakpointData(const BreakpointParameters &params)
{
    if (!b)
        return;
    if (params == b->m_params)
        return;
    b->m_params = params;
    b->destroyMarker();
    b->updateMarker();
    b->update();
    if (b->needsChange() && b->m_engine && b->m_state != BreakpointNew) {
        b->setState(BreakpointChangeRequested);
        b->m_handler->scheduleSynchronization();
    }
}

//////////////////////////////////////////////////////////////////
//
// Storage
//
//////////////////////////////////////////////////////////////////

// Ok to be not thread-safe. The order does not matter and only the gui
// produces authoritative ids.
static int currentId = 0;

BreakpointItem::BreakpointItem(BreakHandler *handler)
    : m_handler(handler), m_id(++currentId), m_state(BreakpointNew), m_engine(0), m_marker(0)
{}

BreakpointItem::~BreakpointItem()
{
    delete m_marker;
}

void BreakpointItem::destroyMarker()
{
    if (m_engine)
        m_engine->updateBreakpointMarkers();
    if (m_marker) {
        BreakpointMarker *m = m_marker;
        m->m_bp = 0;
        m_marker = 0;
        delete m;
    }
}

QString BreakpointItem::markerFileName() const
{
    // Some heuristics to find a "good" file name.
    if (!m_params.fileName.isEmpty()) {
        QFileInfo fi(m_params.fileName);
        if (fi.exists())
            return fi.absoluteFilePath();
    }
    if (!m_response.fileName.isEmpty()) {
        QFileInfo fi(m_response.fileName);
        if (fi.exists())
            return fi.absoluteFilePath();
    }
    if (m_response.fileName.endsWith(m_params.fileName))
        return m_response.fileName;
    if (m_params.fileName.endsWith(m_response.fileName))
        return m_params.fileName;
    return m_response.fileName.size() > m_params.fileName.size()
        ? m_response.fileName : m_params.fileName;
}

int BreakpointItem::markerLineNumber() const
{
    return m_response.lineNumber ? m_response.lineNumber : m_params.lineNumber;
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

bool BreakpointItem::needsChange() const
{
    if (!m_params.conditionsMatch(m_response.condition))
        return true;
    if (m_params.ignoreCount != m_response.ignoreCount)
        return true;
    if (m_params.enabled != m_response.enabled)
        return true;
    if (m_params.threadSpec != m_response.threadSpec)
        return true;
    if (m_params.command != m_response.command)
        return true;
    if (m_params.type == BreakpointByFileAndLine && m_params.lineNumber != m_response.lineNumber)
        return true;
    // FIXME: Too strict, functions may have parameter lists, or not.
    // if (m_params.type == BreakpointByFunction && m_params.functionName != m_response.functionName)
    //     return true;
    // if (m_params.type == BreakpointByAddress && m_params.address != m_response.address)
    //     return true;
    return false;
}

bool BreakpointItem::isLocatedAt
    (const QString &fileName, int lineNumber, bool useMarkerPosition) const
{
    int line = useMarkerPosition ? m_response.lineNumber : m_params.lineNumber;
    return lineNumber == line
        && (fileNameMatch(fileName, m_response.fileName)
            || fileNameMatch(fileName, markerFileName()));
}

void BreakpointItem::updateMarkerIcon()
{
    if (m_marker) {
        m_marker->setIcon(icon());
        m_marker->updateMarker();
    }
}

void BreakpointItem::updateMarker()
{
    QString file = markerFileName();
    int line = markerLineNumber();
    if (m_marker && (file != m_marker->fileName() || line != m_marker->lineNumber()))
        destroyMarker();

    if (!m_marker && !file.isEmpty() && line > 0)
        m_marker = new BreakpointMarker(this, file, line);
}

QIcon BreakpointItem::icon() const
{
    // FIXME: This seems to be called on each cursor blink as soon as the
    // cursor is near a line with a breakpoint marker (+/- 2 lines or so).
    if (m_params.isTracepoint())
        return BreakHandler::tracepointIcon();
    if (m_params.type == WatchpointAtAddress)
        return BreakHandler::watchpointIcon();
    if (m_params.type == WatchpointAtExpression)
        return BreakHandler::watchpointIcon();
    if (!m_params.enabled)
        return BreakHandler::disabledBreakpointIcon();
    if (m_state == BreakpointInserted)
        return BreakHandler::breakpointIcon();
    return BreakHandler::pendingBreakpointIcon();
}

QString BreakpointItem::toToolTip() const
{
    QString rc;
    QTextStream str(&rc);
    str << "<html><body><table>"
        //<< "<tr><td>" << tr("ID:") << "</td><td>" << m_id << "</td></tr>"
        << "<tr><td>" << tr("State:")
        << "</td><td>" << (m_params.enabled ? tr("Enabled") : tr("Disabled"));
    if (m_response.pending)
        str << tr(", pending");
    str << ", " << m_state << "   (" << stateToString(m_state) << ")</td></tr>";
    if (m_engine) {
        str << "<tr><td>" << tr("Engine:")
        << "</td><td>" << m_engine->objectName() << "</td></tr>";
    }
    if (!m_response.pending) {
        str << "<tr><td>" << tr("Breakpoint Number:")
            << "</td><td>" << m_response.id.toString() << "</td></tr>";
    }
    str << "<tr><td>" << tr("Breakpoint Type:")
        << "</td><td>" << typeToString(m_params.type) << "</td></tr>"
        << "<tr><td>" << tr("Marker File:")
        << "</td><td>" << QDir::toNativeSeparators(markerFileName()) << "</td></tr>"
        << "<tr><td>" << tr("Marker Line:")
        << "</td><td>" << markerLineNumber() << "</td></tr>"
        << "</table><br><hr><table>"
        << "<tr><th>" << tr("Property")
        << "</th><th>" << tr("Requested")
        << "</th><th>" << tr("Obtained") << "</th></tr>"
        << "<tr><td>" << tr("Internal Number:")
        << "</td><td>&mdash;</td><td>" << m_response.id.toString() << "</td></tr>";
    if (m_params.type == BreakpointByFunction) {
        str << "<tr><td>" << tr("Function Name:")
        << "</td><td>" << m_params.functionName
        << "</td><td>" << m_response.functionName
        << "</td></tr>";
    }
    if (m_params.type == BreakpointByFileAndLine) {
    str << "<tr><td>" << tr("File Name:")
        << "</td><td>" << QDir::toNativeSeparators(m_params.fileName)
        << "</td><td>" << QDir::toNativeSeparators(m_response.fileName)
        << "</td></tr>"
        << "<tr><td>" << tr("Line Number:")
        << "</td><td>" << m_params.lineNumber
        << "</td><td>" << m_response.lineNumber << "</td></tr>"
        << "<tr><td>" << tr("Corrected Line Number:")
        << "</td><td>-"
        << "</td><td>" << m_response.correctedLineNumber << "</td></tr>";
    }
    if (m_params.type == BreakpointByFunction || m_params.type == BreakpointByFileAndLine) {
        str << "<tr><td>" << tr("Module:")
            << "</td><td>" << m_params.module
            << "</td><td>" << m_response.module
            << "</td></tr>";
    }
    str << "<tr><td>" << tr("Breakpoint Address:")
        << "</td><td>";
    formatAddress(str, m_params.address);
    str << "</td><td>";
    formatAddress(str, m_response.address);
    str << "</td></tr>";
    if (m_response.multiple) {
        str << "<tr><td>" << tr("Multiple Addresses:")
            << "</td><td>"
            << "</td></tr>";
    }
    if (!m_params.command.isEmpty() || !m_response.command.isEmpty()) {
        str << "<tr><td>" << tr("Command:")
            << "</td><td>" << m_params.command
            << "</td><td>" << m_response.command
            << "</td></tr>";
    }
    if (!m_params.message.isEmpty() || !m_response.message.isEmpty()) {
        str << "<tr><td>" << tr("Message:")
            << "</td><td>" << m_params.message
            << "</td><td>" << m_response.message
            << "</td></tr>";
    }
    if (!m_params.condition.isEmpty() || !m_response.condition.isEmpty()) {
        str << "<tr><td>" << tr("Condition:")
            << "</td><td>" << m_params.condition
            << "</td><td>" << m_response.condition
            << "</td></tr>";
    }
    if (m_params.ignoreCount || m_response.ignoreCount) {
        str << "<tr><td>" << tr("Ignore Count:") << "</td><td>";
        if (m_params.ignoreCount)
            str << m_params.ignoreCount;
        str << "</td><td>";
        if (m_response.ignoreCount)
            str << m_response.ignoreCount;
        str << "</td></tr>";
    }
    if (m_params.threadSpec >= 0 || m_response.threadSpec >= 0) {
        str << "<tr><td>" << tr("Thread Specification:")
            << "</td><td>";
        if (m_params.threadSpec >= 0)
            str << m_params.threadSpec;
        str << "</td><td>";
        if (m_response.threadSpec >= 0)
            str << m_response.threadSpec;
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
    if (findWatchpoint(params)) {
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
    if (findWatchpoint(params)) {
        qDebug() << "WATCHPOINT EXISTS";
        //   removeBreakpoint(index);
        return;
    }
    appendBreakpoint(params);
}

bool Breakpoint::isValid() const
{
    return b && b->m_id.isValid();
}

uint Breakpoint::hash() const
{
    return b ? 0 : qHash(b->m_id);
}

BreakpointModelId Breakpoint::id() const
{
    return b ? b->m_id : BreakpointModelId();
}

QString Breakpoint::msgWatchpointByExpressionTriggered(const int number, const QString &expr) const
{
    return id()
        ? tr("Data breakpoint %1 (%2) at %3 triggered.")
            .arg(id().toString()).arg(number).arg(expr)
        : tr("Internal data breakpoint %1 at %2 triggered.")
            .arg(number).arg(expr);
}

QString Breakpoint::msgWatchpointByExpressionTriggered(const int number, const QString &expr,
                                                       const QString &threadId) const
{
    return id()
        ? tr("Data breakpoint %1 (%2) at %3 in thread %4 triggered.")
            .arg(id().toString()).arg(number).arg(expr).arg(threadId)
        : tr("Internal data breakpoint %1 at %2 in thread %3 triggered.")
            .arg(number).arg(expr).arg(threadId);
}

QString Breakpoint::msgWatchpointByAddressTriggered(const int number, quint64 address) const
{
    return id()
        ? tr("Data breakpoint %1 (%2) at 0x%3 triggered.")
            .arg(id().toString()).arg(number).arg(address, 0, 16)
        : tr("Internal data breakpoint %1 at 0x%2 triggered.")
            .arg(number).arg(address, 0, 16);
}

QString Breakpoint::msgWatchpointByAddressTriggered(
    const int number, quint64 address, const QString &threadId) const
{
    return id()
        ? tr("Data breakpoint %1 (%2) at 0x%3 in thread %4 triggered.")
            .arg(id().toString()).arg(number).arg(address, 0, 16).arg(threadId)
        : tr("Internal data breakpoint %1 at 0x%2 in thread %3 triggered.")
            .arg(id().toString()).arg(number).arg(address, 0, 16).arg(threadId);
}

QString Breakpoint::msgBreakpointTriggered(const int number, const QString &threadId) const
{
    return id()
        ? tr("Stopped at breakpoint %1 (%2) in thread %3.")
            .arg(id().toString()).arg(number).arg(threadId)
        : tr("Stopped at internal breakpoint %1 in thread %2.")
            .arg(number).arg(threadId);
}

} // namespace Internal
} // namespace Debugger
