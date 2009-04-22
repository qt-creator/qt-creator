/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cdbbreakpoint.h"
#include "cdbmodules.h"
#include "breakhandler.h"
#include "cdbdebugengine_p.h"

#include <QtCore/QTextStream>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QMap>

namespace Debugger {
namespace Internal {

// The CDB breakpoint expression syntax is:
//    `foo.cpp:523`[ "condition"]
//    module!function[ "condition"]

static const char sourceFileQuoteC = '`';

CDBBreakPoint::CDBBreakPoint() :
    ignoreCount(0),
    lineNumber(-1)
{
}

CDBBreakPoint::CDBBreakPoint(const BreakpointData &bpd) :
     fileName(bpd.fileName),
     condition(bpd.condition),
     ignoreCount(0),
     funcName(bpd.funcName),
     lineNumber(-1)
{
    if (!bpd.ignoreCount.isEmpty())
        ignoreCount = bpd.ignoreCount.toInt();
    if (!bpd.lineNumber.isEmpty())
        lineNumber = bpd.lineNumber.toInt();
}

int CDBBreakPoint::compare(const CDBBreakPoint& rhs) const
{
    if (ignoreCount > rhs.ignoreCount)
        return 1;
    if (ignoreCount < rhs.ignoreCount)
        return -1;
    if (lineNumber > rhs.lineNumber)
        return 1;
    if (lineNumber < rhs.lineNumber)
        return -1;
    if (const int fileCmp = fileName.compare(rhs.fileName))
        return fileCmp;
    if (const int  funcCmp = funcName.compare(rhs.funcName))
        return funcCmp;
    if (const int condCmp = condition.compare(rhs.condition))
        return condCmp;
    return 0;
}

void CDBBreakPoint::clear()
{
     ignoreCount = 0;
     clearExpressionData();
}

void CDBBreakPoint::clearExpressionData()
{
    fileName.clear();
    condition.clear();
    funcName.clear();
    lineNumber = -1;
}

QDebug operator<<(QDebug dbg, const CDBBreakPoint &bp)
{
    QDebug nsp = dbg.nospace();
    if (!bp.fileName.isEmpty()) {
        nsp << "fileName='" << bp.fileName << ':' << bp.lineNumber << '\'';
    } else {
        nsp << "funcName='" << bp.funcName << '\'';
    }
    if (!bp.condition.isEmpty())
        nsp << " condition='" << bp.condition << '\'';
    if (bp.ignoreCount)
        nsp << " ignoreCount=" << bp.ignoreCount;
    return dbg;
}

QString CDBBreakPoint::expression() const
{
    // format the breakpoint expression (file/function and condition)
    QString rc;
    QTextStream str(&rc);
    if (funcName.isEmpty()) {
        const QChar sourceFileQuote = QLatin1Char(sourceFileQuoteC);
        str << sourceFileQuote << QDir::toNativeSeparators(fileName) << QLatin1Char(':') << lineNumber << sourceFileQuote;
    } else {
        str << funcName;
    }
    if (!condition.isEmpty()) {
        const QChar doubleQuote = QLatin1Char('"');
        str << QLatin1Char(' ') << doubleQuote << condition << doubleQuote;
    }
    return rc;
}

bool CDBBreakPoint::apply(CIDebugBreakpoint *ibp, QString *errorMessage) const
{
    const QString expr = expression();
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << *this << expr;
    const HRESULT hr = ibp->SetOffsetExpressionWide(expr.utf16());
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Unable to set breakpoint '%1' : %2").
                        arg(expr, msgComFailed("SetOffsetExpressionWide", hr));
        return false;
    }
    // Pass Count is ignoreCount + 1
    ibp->SetPassCount(ignoreCount + 1u);
    ibp->AddFlags(DEBUG_BREAKPOINT_ENABLED);
    return true;
}

bool CDBBreakPoint::add(CIDebugControl* debugControl, QString *errorMessage) const
{
    IDebugBreakpoint2* ibp = 0;
    const HRESULT hr = debugControl->AddBreakpoint2(DEBUG_BREAKPOINT_CODE, DEBUG_ANY_ID, &ibp);
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Unable to add breakpoint: %1").
                        arg(msgComFailed("AddBreakpoint2", hr));
        return false;
    }
    if (!ibp) {
        *errorMessage = QString::fromLatin1("Unable to add breakpoint: <Unknown error>");
        return false;
    }
    return apply(ibp, errorMessage);
}

// Make sure file can be found in editor manager and text markers
// Use '/' and capitalize drive letter
QString CDBBreakPoint::canonicalSourceFile(const QString &f)
{
    if (f.isEmpty())
        return f;
    QString rc = QDir::fromNativeSeparators(f);
    if (rc.size() > 2 && rc.at(1) == QLatin1Char(':'))
        rc[0] = rc.at(0).toUpper();
    return rc;
}

bool CDBBreakPoint::retrieve(CIDebugBreakpoint *ibp, QString *errorMessage)
{
    clear();
    WCHAR wszBuf[MAX_PATH];
    const HRESULT hr =ibp->GetOffsetExpressionWide(wszBuf, MAX_PATH, 0);
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Cannot retrieve breakpoint: %1").
                        arg(msgComFailed("GetOffsetExpressionWide", hr));
        return false;
    }
    // Pass Count is ignoreCount + 1
    ibp->GetPassCount(&ignoreCount);
    if (ignoreCount)
        ignoreCount--;
    const QString expr = QString::fromUtf16(wszBuf);
    if (!parseExpression(expr)) {
        *errorMessage = QString::fromLatin1("Parsing of '%1' failed.").arg(expr);
        return false;
    }
    return true;
}

bool CDBBreakPoint::parseExpression(const QString &expr)
{
    clearExpressionData();
    const QChar sourceFileQuote = QLatin1Char(sourceFileQuoteC);
    // Check for file or function
    int conditionPos = 0;
    if (expr.startsWith(sourceFileQuote)) { // `c:\foo.cpp:523`[ "condition"]
        // Do not fall for the drive letter colon here
        const int colonPos = expr.indexOf(QLatin1Char(':'), 3);
        if (colonPos == -1)
            return false;
        conditionPos = expr.indexOf(sourceFileQuote, colonPos + 1);
        if (conditionPos == -1)
            return false;
        fileName = canonicalSourceFile(expr.mid(1, colonPos - 1));
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

bool CDBBreakPoint::getBreakPointCount(CIDebugControl* debugControl, ULONG *count, QString *errorMessage /* = 0*/)
{
    const HRESULT hr = debugControl->GetNumberBreakpoints(count);
    if (FAILED(hr)) {
        if (errorMessage)
            *errorMessage = QString::fromLatin1("Cannot determine breakpoint count: %1").
                            arg(msgComFailed("GetNumberBreakpoints", hr));
        return false;
    }
    return true;
}

bool CDBBreakPoint::getBreakPoints(CIDebugControl* debugControl, QList<CDBBreakPoint> *bps, QString *errorMessage)
{
    ULONG count = 0;
    bps->clear();
    if (!getBreakPointCount(debugControl, &count, errorMessage))
        return false;
    // retrieve one by one and parse
    for (ULONG b= 0; b < count; b++) {
        IDebugBreakpoint2 *ibp = 0;
        const HRESULT hr = debugControl->GetBreakpointByIndex2(b, &ibp);
        if (FAILED(hr)) {
            *errorMessage = QString::fromLatin1("Cannot retrieve breakpoint %1: %2").
                            arg(b).arg(msgComFailed("GetBreakpointByIndex2", hr));
            return false;
        }
        CDBBreakPoint bp;
        if (!bp.retrieve(ibp, errorMessage))
            return false;
        bps->push_back(bp);
    }
    return true;
}


// Synchronize (halted) engine breakpoints with those of the BreakHandler.
bool CDBBreakPoint::synchronizeBreakPoints(IDebugControl4* debugControl,
                                           IDebugSymbols3 *syms,
                                           BreakHandler *handler,
                                           QString *errorMessage)
{    
    typedef QMap<CDBBreakPoint, int> BreakPointIndexMap;
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    BreakPointIndexMap breakPointIndexMap;
    // convert BreakHandler's bps into a map of BreakPoint->BreakHandler->Index
    // Ignore invalid functions (that could not be found) as they make
    // the debugger hang.
    const int handlerCount = handler->size();
    const QChar moduleDelimiter = QLatin1Char('!');
    for (int i=0; i < handlerCount; ++i) {
        BreakpointData *bd = handler->at(i);
        // Function breakpoints: Are the module names specified?
        bool breakPointOk = false;
        if (bd->funcName.isEmpty()) {
            breakPointOk = true;
        } else {
            switch (resolveSymbol(syms, &bd->funcName, errorMessage)) {
            case ResolveSymbolOk:
                breakPointOk = true;
                break;
            case ResolveSymbolAmbiguous:
                qWarning("Warning: %s\n", qPrintable(*errorMessage));
                breakPointOk = true;
                break;
            case ResolveSymbolNotFound:
            case ResolveSymbolError:
                qWarning("Warning: %s\n", qPrintable(*errorMessage));
                break;
            };
        } // function breakpoint
        if (breakPointOk)
            breakPointIndexMap.insert(CDBBreakPoint(*bd), i);
    }
    errorMessage->clear();
    // get number of engine breakpoints
    ULONG engineCount;
    if (!getBreakPointCount(debugControl, &engineCount, errorMessage))
        return false;

    // Starting from end, check if engine breakpoints are still in handler.
    // If not->remove
    if (engineCount) {
        for (ULONG eb = engineCount - 1u; ; eb--) {
            // get engine breakpoint.
            IDebugBreakpoint2 *ibp = 0;
            HRESULT hr = debugControl->GetBreakpointByIndex2(eb, &ibp);
            if (FAILED(hr)) {
                *errorMessage = QString::fromLatin1("Cannot retrieve breakpoint %1: %2").
                                arg(eb).arg(msgComFailed("GetBreakpointByIndex2", hr));
                return false;
            }
            // Ignore one shot break points set by "Step out"
            ULONG flags = 0;
            hr = ibp->GetFlags(&flags);
            if (!(flags & DEBUG_BREAKPOINT_ONE_SHOT)) {
                CDBBreakPoint engineBreakPoint;
                if (!engineBreakPoint.retrieve(ibp, errorMessage))
                    return false;
                // Still in handler?
                if (!breakPointIndexMap.contains(engineBreakPoint)) {
                    if (debugCDB)
                        qDebug() << "    Removing" << engineBreakPoint;
                    hr = debugControl->RemoveBreakpoint2(ibp);
                    if (FAILED(hr)) {
                        *errorMessage = QString::fromLatin1("Cannot remove breakpoint %1: %2").
                                        arg(engineBreakPoint.expression(), msgComFailed("RemoveBreakpoint2", hr));
                        return false;
                    }
                } // not in handler
            } // one shot
            if (!eb)
                break;
        }
    }
    // Add pending breakpoints
    const BreakPointIndexMap::const_iterator pcend = breakPointIndexMap.constEnd();
    for (BreakPointIndexMap::const_iterator it = breakPointIndexMap.constBegin(); it != pcend; ++it) {
        const int index = it.value();
        if (handler->at(index)->pending) {
            if (debugCDB)
                qDebug() << "    Adding " << it.key();
            if (it.key().add(debugControl, errorMessage)) {
                handler->at(index)->pending = false;
            } else {
                const QString msg = QString::fromLatin1("Failed to add breakpoint '%1': %2").arg(it.key().expression(), *errorMessage);
                qWarning("%s\n", qPrintable(msg));
            }
        }
    }
    if (debugCDB > 1) {
        QList<CDBBreakPoint> bps;
        CDBBreakPoint::getBreakPoints(debugControl, &bps, errorMessage);
        qDebug().nospace() << "### Breakpoints in engine: " << bps;
    }
    return true;
}

} // namespace Internal
} // namespace Debugger
