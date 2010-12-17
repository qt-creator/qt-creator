/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "corebreakpoint.h"
#include "coreengine.h"
#ifndef TEST_COMPILE // Usage in manual tests
#    include "dbgwinutils.h"
#endif
#include <utils/qtcassert.h>

#include <QtCore/QTextStream>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QMap>

enum { debugBP = 0 };

namespace CdbCore {

// The CDB breakpoint expression syntax is:
//    `foo.cpp:523`[ "condition"]
//    module!function[ "condition"]

static const char sourceFileQuoteC = '`';

BreakPoint::BreakPoint() :
    type(Code),
    lineNumber(-1),
    address(0),
    threadId(-1),
    ignoreCount(0),
    oneShot(false),
    enabled(true)
{
}

void BreakPoint::clear()
{
     type = Code;
     ignoreCount = 0;
     threadId = -1;
     oneShot = false;
     enabled = true;
     clearExpressionData();
}

void BreakPoint::clearExpressionData()
{
    fileName.clear();
    condition.clear();
    funcName.clear();
    lineNumber = -1;
    address = 0;
}

QDebug operator<<(QDebug dbg, const BreakPoint &bp)
{
    dbg.nospace() << bp.toString();
    return dbg;
}

QString BreakPoint::toString() const
{
    QString rc;
    QTextStream str(&rc);
    str << (type == BreakPoint::Code ?  "Code " : "Data ");
    if (address) {
        str.setIntegerBase(16);
        str << "0x" << address << ' ';
        str.setIntegerBase(10);
    }
    if (!fileName.isEmpty()) {
        str << "fileName='" << fileName << ':' << lineNumber << '\'';
    } else {
        str << "funcName='" << funcName << '\'';
    }
    if (threadId >= 0)
        str << " thread=" << threadId;
    if (!condition.isEmpty())
        str << " condition='" << condition << '\'';
    if (ignoreCount)
        str << " ignoreCount=" << ignoreCount;
    str << (enabled ? " enabled" : " disabled");
    if (oneShot)
        str << " oneShot";
    return rc;
}

QString BreakPoint::expression() const
{
    // format the breakpoint expression (file/function and condition)
    QString rc;
    QTextStream str(&rc);
    do {
        if (address) {
            str.setIntegerBase(16);
            str << "0x" << address;
            str.setIntegerBase(10);
            break;
        }
        if (!fileName.isEmpty()) {
            const QChar sourceFileQuote = QLatin1Char(sourceFileQuoteC);
            str << sourceFileQuote << QDir::toNativeSeparators(fileName) << QLatin1Char(':')
                    << lineNumber << sourceFileQuote;
            break;
        }
        if (!funcName.isEmpty()) {
            str << funcName;
            break;
        }
    } while (false);
    if (!condition.isEmpty())
        str << " \"" << condition << '"';
    return rc;
}

static inline QString msgCannotSetBreakpoint(const QString &exp, const QString &why)
{
    return QString::fromLatin1("Unable to set breakpoint '%1' : %2").arg(exp, why);
}

bool BreakPoint::apply(CIDebugBreakpoint *ibp, QString *errorMessage) const
{
    const QString expr = expression();
    if (debugBP)
        qDebug() << Q_FUNC_INFO << *this << expr;
    HRESULT hr = ibp->SetOffsetExpressionWide(reinterpret_cast<PCWSTR>(expr.utf16()));
    if (FAILED(hr)) {
        *errorMessage = msgCannotSetBreakpoint(expr, CdbCore::msgComFailed("SetOffsetExpressionWide", hr));
        return false;
    }
    // Pass Count is ignoreCount + 1
    hr = ibp->SetPassCount(ignoreCount + 1u);
    if (FAILED(hr))
        qWarning("Error setting passcount %d %s ", ignoreCount, qPrintable(expr));
    // Set up size for data breakpoints
    if (type == Data) {
        const ULONG size = 1u;
        hr = ibp->SetDataParameters(size, DEBUG_BREAK_READ | DEBUG_BREAK_WRITE);
        if (FAILED(hr)) {
            const QString msg = QString::fromLatin1("Cannot set watch size to %1: %2").
                                arg(size).arg(CdbCore::msgComFailed("SetDataParameters", hr));
            *errorMessage = msgCannotSetBreakpoint(expr, msg);
            return false;
        }
    }
    // Thread
    if (threadId >= 0) {
        hr = ibp->SetMatchThreadId(threadId);
        if (FAILED(hr)) {
            const QString msg = QString::fromLatin1("Cannot set thread id to %1: %2").
                                arg(threadId).arg(CdbCore::msgComFailed("SetMatchThreadId", hr));
            *errorMessage = msgCannotSetBreakpoint(expr, msg);
            return false;
        }
    }
    // Flags
    ULONG flags = 0;
    if (enabled)
        flags |= DEBUG_BREAKPOINT_ENABLED;
    if (oneShot)
        flags |= DEBUG_BREAKPOINT_ONE_SHOT;
    hr = ibp->AddFlags(flags);
    if (FAILED(hr)) {
        const QString msg = QString::fromLatin1("Cannot set flags to 0x%1: %2").
                            arg(flags, 0 ,16).arg(CdbCore::msgComFailed("AddFlags", hr));
        *errorMessage = msgCannotSetBreakpoint(expr, msg);
        return false;
    }
    return true;
}

static inline QString msgCannotAddBreakPoint(const QString &why)
{
    return QString::fromLatin1("Unable to add breakpoint: %1").arg(why);
}

bool BreakPoint::add(CIDebugControl* debugControl,
                        QString *errorMessage,
                        unsigned long *id,
                        quint64 *address) const
{
    CIDebugBreakpoint* ibp = 0;
    if (address)
        *address = 0;
    if (id)
        *id = 0;
    const ULONG iType = type == Code ? DEBUG_BREAKPOINT_CODE : DEBUG_BREAKPOINT_DATA;
    HRESULT hr = debugControl->AddBreakpoint2(iType, DEBUG_ANY_ID, &ibp);
    if (FAILED(hr)) {
        *errorMessage = msgCannotAddBreakPoint(CdbCore::msgComFailed("AddBreakpoint2", hr));
        return false;
    }
    if (!ibp) {
        *errorMessage = msgCannotAddBreakPoint(QLatin1String("<Unknown error>"));
        return false;
    }
    if (!apply(ibp, errorMessage))
        return false;
    // GetOffset can fail when attaching to remote processes, ignore return
    if (address) {
        hr = ibp->GetOffset(address);
        if (FAILED(hr))
            *address = 0;
    }
    if (id) {
        hr = ibp->GetId(id);
        if (FAILED(hr)) {
            *errorMessage = msgCannotAddBreakPoint(CdbCore::msgComFailed("GetId", hr));
            return false;
        }
    }
    return true;
}

// Make sure file can be found in editor manager and text markers
// Use '/', correct case and capitalize drive letter. Use a cache.

typedef QHash<QString, QString> NormalizedFileCache;
Q_GLOBAL_STATIC(NormalizedFileCache, normalizedFileNameCache)

QString BreakPoint::normalizeFileName(const QString &f)
{
#ifdef TEST_COMPILE // Usage in manual tests
    return f;
#else
    QTC_ASSERT(!f.isEmpty(), return f)
    const NormalizedFileCache::const_iterator it = normalizedFileNameCache()->constFind(f);
    if (it != normalizedFileNameCache()->constEnd())
        return it.value();
    QString normalizedName = QDir::fromNativeSeparators(Debugger::Internal::winNormalizeFileName(f));
    // Upcase drive letter for consistency even if case mapping fails.
    if (normalizedName.size() > 2 && normalizedName.at(1) == QLatin1Char(':'))
        normalizedName[0] = normalizedName.at(0).toUpper();
    normalizedFileNameCache()->insert(f, normalizedName);
    return normalizedName;
#endif
}

void BreakPoint::clearNormalizeFileNameCache()
{
    normalizedFileNameCache()->clear();
}

static inline QString msgCannotRetrieveBreakpoint(const QString &why)
{
    return QString::fromLatin1("Cannot retrieve breakpoint: %1").arg(why);
}

static inline int threadIdOfBreakpoint(CIDebugBreakpoint *ibp)
{
    // Thread: E_NOINTERFACE indicates no thread has been set.
    int threadId = -1;
    ULONG iThreadId;
    if (S_OK == ibp->GetMatchThreadId(&iThreadId))
        threadId = iThreadId;
    return threadId;
}

bool BreakPoint::retrieve(CIDebugBreakpoint *ibp, QString *errorMessage)
{
    clear();
    // Get type
    ULONG iType;
    ULONG processorType;
    HRESULT hr = ibp->GetType(&iType, &processorType);
    if (FAILED(hr)) {
        *errorMessage = msgCannotRetrieveBreakpoint(CdbCore::msgComFailed("GetType", hr));
        return false;
    }
    type = iType == DEBUG_BREAKPOINT_CODE ? Code : Data;
    // Get & parse expression
    WCHAR wszBuf[MAX_PATH];
    hr =ibp->GetOffsetExpressionWide(wszBuf, MAX_PATH, 0);
    if (FAILED(hr)) {
        *errorMessage = msgCannotRetrieveBreakpoint(CdbCore::msgComFailed("GetOffsetExpressionWide", hr));
        return false;
    }
    threadId = threadIdOfBreakpoint(ibp);
    // Pass Count is ignoreCount + 1
    ibp->GetPassCount(&ignoreCount);
    if (ignoreCount)
        ignoreCount--;
    ULONG flags = 0;
    ibp->GetFlags(&flags);
    oneShot = (flags & DEBUG_BREAKPOINT_ONE_SHOT);
    enabled = (flags & DEBUG_BREAKPOINT_ENABLED);
    const QString expr = QString::fromUtf16(reinterpret_cast<const ushort *>(wszBuf));
    if (!parseExpression(expr)) {
        *errorMessage = QString::fromLatin1("Parsing of '%1' failed.").arg(expr);
        return false;
    }
    return true;
}

bool BreakPoint::parseExpression(const QString &expr)
{
    clearExpressionData();
    const QChar sourceFileQuote = QLatin1Char(sourceFileQuoteC);
    // Check for file or function
    int conditionPos = 0;
    if (expr.startsWith(QLatin1String("0x"))) { // Check address token
        conditionPos = expr.indexOf(QLatin1Char(' '));
        QString addressS;
        if (conditionPos != -1) {
            addressS = expr.mid(2, conditionPos - 2);
            conditionPos++;
        } else {
            addressS = expr.mid(2);
            conditionPos = expr.size();
        }
        addressS.remove(QLatin1Char('\'')); // 64bit separator
        bool ok;
        address = addressS.toULongLong(&ok, 16);
        if (!ok)
            return false;
    } else if (expr.startsWith(sourceFileQuote)) { // `c:\foo.cpp:523`[ "condition"]
        // Do not fall for the drive letter colon here
        const int colonPos = expr.indexOf(QLatin1Char(':'), 3);
        if (colonPos == -1)
            return false;
        conditionPos = expr.indexOf(sourceFileQuote, colonPos + 1);
        if (conditionPos == -1)
            return false;
        fileName = normalizeFileName(expr.mid(1, colonPos - 1));
        const QString lineNumberS = expr.mid(colonPos + 1, conditionPos - colonPos - 1);
        bool lineNumberOk = false;
        lineNumber = lineNumberS.toInt(&lineNumberOk);
        if (!lineNumberOk)
            return false;
        conditionPos++;
    } else {
        // Check function token
        conditionPos = expr.indexOf(QLatin1Char(' '));
        if (conditionPos != -1) {
            funcName = expr.mid(0, conditionPos);
            conditionPos++;
        } else {
            funcName = expr;
            conditionPos = expr.size();
        }
    }
    // Condition? ".if bla"
    if (conditionPos >= expr.size())
        return true;
    const QChar doubleQuote = QLatin1Char('"');
    conditionPos = expr.indexOf(doubleQuote, conditionPos);
    if (conditionPos == -1)
        return true;
    conditionPos++;
    const int condEndPos = expr.lastIndexOf(doubleQuote);
    if (condEndPos == -1)
        return false;
    condition = expr.mid(conditionPos, condEndPos - conditionPos);
    return true;
}

bool BreakPoint::getBreakPointCount(CIDebugControl* debugControl, ULONG *count, QString *errorMessage /* = 0*/)
{
    const HRESULT hr = debugControl->GetNumberBreakpoints(count);
    if (FAILED(hr)) {
        if (errorMessage)
            *errorMessage = QString::fromLatin1("Cannot determine breakpoint count: %1").
                            arg(CdbCore::msgComFailed("GetNumberBreakpoints", hr));
        return false;
    }
    return true;
}

bool BreakPoint::getBreakPoints(CIDebugControl* debugControl, QList<BreakPoint> *bps, QString *errorMessage)
{
    ULONG count = 0;
    bps->clear();
    if (!getBreakPointCount(debugControl, &count, errorMessage))
        return false;
    // retrieve one by one and parse
    for (ULONG b = 0; b < count; b++) {
        CIDebugBreakpoint *ibp = 0;
        const HRESULT hr = debugControl->GetBreakpointByIndex2(b, &ibp);
        if (FAILED(hr)) {
            *errorMessage = QString::fromLatin1("Cannot retrieve breakpoint %1: %2").
                            arg(b).arg(CdbCore::msgComFailed("GetBreakpointByIndex2", hr));
            return false;
        }
        BreakPoint bp;
        if (!bp.retrieve(ibp, errorMessage))
            return false;
        bps->push_back(bp);
    }
    return true;
}

// Find a breakpoint by id
static inline QString msgNoBreakPointWithId(unsigned long id, const QString &why)
{
    return QString::fromLatin1("Unable to find breakpoint with id %1: %2").arg(id).arg(why);
}

CIDebugBreakpoint *BreakPoint::breakPointById(CIDebugControl *ctl, unsigned long id, QString *errorMessage)
{
    CIDebugBreakpoint *ibp = 0;
    const HRESULT hr = ctl->GetBreakpointById2(id, &ibp);
    if (FAILED(hr)) {
        *errorMessage = msgNoBreakPointWithId(id, CdbCore::msgComFailed("GetBreakpointById2", hr));
        return 0;
    }
    if (!ibp) {
        *errorMessage = msgNoBreakPointWithId(id, QLatin1String("<not found>"));
        return 0;
    }
    return ibp;
}

// Remove breakpoint by id
bool BreakPoint::removeBreakPointById(CIDebugControl *ctl, unsigned long id, QString *errorMessage)
{
    if (debugBP)
        qDebug() << Q_FUNC_INFO << id;
    CIDebugBreakpoint *ibp = breakPointById(ctl, id, errorMessage);
    if (!ibp)
        return false;
    const HRESULT hr = ctl->RemoveBreakpoint2(ibp);
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Cannot remove breakpoint %1: %2").arg(id).arg(CdbCore::msgComFailed("RemoveBreakpointById2", hr));
        return false;
    }
    return true;
}

// Set enabled by id

// Change enabled state of a breakpoint by id
static inline QString msgCannotSetBreakPointEnabled(unsigned long id, bool enabled, const QString &why)
{
    return QString::fromLatin1("Cannot %1 breakpoint %2: %3").
            arg(QLatin1String(enabled ? "enable" : "disable")).arg(id).arg(why);
}

bool BreakPoint::setBreakPointEnabledById(CIDebugControl *ctl, unsigned long id, bool enabled, QString *errorMessage)
{
    if (debugBP)
        qDebug() << Q_FUNC_INFO << id << enabled;
    CIDebugBreakpoint *ibp = breakPointById(ctl, id, errorMessage);
    if (!ibp) {
        *errorMessage = msgCannotSetBreakPointEnabled(id, enabled, *errorMessage);
        return false;
    }
    // Compare flags
    ULONG flags;
    HRESULT hr = ibp->GetFlags(&flags);
    if (FAILED(hr)) {
        *errorMessage = msgCannotSetBreakPointEnabled(id, enabled, CdbCore::msgComFailed("GetFlags", hr));
        return false;
    }
    const bool wasEnabled = (flags & DEBUG_BREAKPOINT_ENABLED);
    if (wasEnabled == enabled)
        return true;
    // Set new value
    if (enabled) {
        flags |= DEBUG_BREAKPOINT_ENABLED;
    } else {
        flags &= ~DEBUG_BREAKPOINT_ENABLED;
    }
    hr = ibp->SetFlags(flags);
    if (FAILED(hr)) {
        *errorMessage = msgCannotSetBreakPointEnabled(id, enabled, CdbCore::msgComFailed("SetFlags", hr));
        return false;
    }
    return true;
}

// Change thread-id of a breakpoint
static inline QString msgCannotSetBreakPointThread(unsigned long id, int tid, const QString &why)
{
    return QString::fromLatin1("Cannot set breakpoint %1 thread to %2: %3").arg(id).arg(tid).arg(why);
}

bool BreakPoint::setBreakPointThreadById(CIDebugControl *ctl, unsigned long id, int threadId, QString *errorMessage)
{
    if (debugBP)
        qDebug() << Q_FUNC_INFO << id << threadId;
    CIDebugBreakpoint *ibp = breakPointById(ctl, id, errorMessage);
    if (!ibp) {
        *errorMessage = msgCannotSetBreakPointThread(id, threadId, *errorMessage);
        return false;
    }
    // Compare thread ids
    const int oldThreadId = threadIdOfBreakpoint(ibp);
    if (oldThreadId == threadId)
        return true;
    const ULONG newIThreadId = threadId == -1 ? DEBUG_ANY_ID : static_cast<ULONG>(threadId);
    if (debugBP)
        qDebug() << "Changing thread id of " << id << " from " << oldThreadId << " to " << threadId
                << '(' << newIThreadId << ')';
    const HRESULT hr = ibp->SetMatchThreadId(newIThreadId);
    if (FAILED(hr)) {
        *errorMessage = msgCannotSetBreakPointThread(id, threadId, *errorMessage);
        return false;
    }
    return true;
}
} // namespace CdbCore
