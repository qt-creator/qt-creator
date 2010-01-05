/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cdbbreakpoint.h"
#include "cdbmodules.h"
#include "breakhandler.h"
#include "cdbdebugengine_p.h"

#include <QtCore/QTextStream>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QMap>

#include <psapi.h>

enum { debugBP = 0 };

namespace Debugger {
namespace Internal {

// The CDB breakpoint expression syntax is:
//    `foo.cpp:523`[ "condition"]
//    module!function[ "condition"]

static const char sourceFileQuoteC = '`';

CDBBreakPoint::CDBBreakPoint() :
    ignoreCount(0),
    lineNumber(-1),
    oneShot(false),
    enabled(true)
{
}

CDBBreakPoint::CDBBreakPoint(const BreakpointData &bpd) :
     fileName(QDir::toNativeSeparators(bpd.fileName)),
     condition(bpd.condition),
     ignoreCount(0),
     funcName(bpd.funcName),
     lineNumber(-1),
     oneShot(false),
     enabled(bpd.enabled)
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
    if (oneShot && !rhs.oneShot)
        return 1;
    if (!oneShot && rhs.oneShot)
        return -1;
    if (enabled && !rhs.enabled)
        return 1;
    if (!enabled && rhs.enabled)
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
     oneShot = false;
     enabled = true;
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
    if (bp.enabled)
        nsp << " enabled";
    if (bp.oneShot)
        nsp << " oneShot";
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
    const HRESULT hr = ibp->SetOffsetExpressionWide(reinterpret_cast<PCWSTR>(expr.utf16()));
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Unable to set breakpoint '%1' : %2").
                        arg(expr, msgComFailed("SetOffsetExpressionWide", hr));
        return false;
    }
    // Pass Count is ignoreCount + 1
    ibp->SetPassCount(ignoreCount + 1u);
    ULONG flags = 0;
    if (enabled)
        flags |= DEBUG_BREAKPOINT_ENABLED;
    if (oneShot)
        flags |= DEBUG_BREAKPOINT_ONE_SHOT;
    ibp->AddFlags(flags);
    return true;
}

static inline QString msgCannotAddBreakPoint(const QString &why)
{
    return QString::fromLatin1("Unable to add breakpoint: %1").arg(why);
}

bool CDBBreakPoint::add(CIDebugControl* debugControl,
                        QString *errorMessage,
                        unsigned long *id,
                        quint64 *address) const
{
    IDebugBreakpoint2* ibp = 0;
    if (address)
        *address = 0;
    if (id)
        *id = 0;
    HRESULT hr = debugControl->AddBreakpoint2(DEBUG_BREAKPOINT_CODE, DEBUG_ANY_ID, &ibp);
    if (FAILED(hr)) {
        *errorMessage = msgCannotAddBreakPoint(msgComFailed("AddBreakpoint2", hr));
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
            *errorMessage = msgCannotAddBreakPoint(msgComFailed("GetId", hr));
            return false;
        }
    }
    return true;
}

// Helper for normalizing file names:
// Map the device paths in  a file name to back to drive letters
// "/Device/HarddiskVolume1/file.cpp" -> "C:/file.cpp"

static bool mapDeviceToDriveLetter(QString *s)
{
    enum { bufSize = 512 };
    // Retrieve drive letters and get their device names.
    // Do not cache as it may change due to removable/network drives.
    TCHAR driveLetters[bufSize];
    if (!GetLogicalDriveStrings(bufSize-1, driveLetters))
        return false;

    TCHAR driveName[MAX_PATH];
    TCHAR szDrive[3] = TEXT(" :");
    for (const TCHAR *driveLetter = driveLetters; *driveLetter; driveLetter++) {
        szDrive[0] = *driveLetter; // Look up each device name
        if (QueryDosDevice(szDrive, driveName, MAX_PATH)) {
            const QString deviceName = QString::fromUtf16(driveName);
            if (s->startsWith(deviceName)) {
                s->replace(0, deviceName.size(), QString::fromUtf16(szDrive));
                return true;
            }
        }
    }
    return false;
}

// Helper for normalizing file names:
// Determine normalized case of a Windows file name (camelcase.cpp -> CamelCase.cpp)
// as the debugger reports lower case file names.
// Restriction: File needs to exists and be non-empty and will be to be opened/mapped.
// This is the MSDN-recommended way of doing that. The result should be cached.

static inline QString normalizeFileNameCaseHelper(const QString &f)
{
    HANDLE hFile = CreateFile(f.utf16(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return f;
    // Get the file size. We need a non-empty file to map it.
    DWORD dwFileSizeHi = 0;
    DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi);
    if (dwFileSizeLo == 0 && dwFileSizeHi == 0) {
        CloseHandle(hFile);
        return f;
    }
    // Create a file mapping object.
    HANDLE hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 1, NULL);
    if (!hFileMap)  {
        CloseHandle(hFile);
        return f;
    }

    // Create a file mapping to get the file name.
    void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);
    if (!pMem) {
        CloseHandle(hFileMap);
        CloseHandle(hFile);
        return f;
    }

    QString rc;
    WCHAR pszFilename[MAX_PATH];
    pszFilename[0] = 0;
    // Get a file name of the form "/Device/HarddiskVolume1/file.cpp"
    if (GetMappedFileName (GetCurrentProcess(), pMem, pszFilename, MAX_PATH)) {
        rc = QString::fromUtf16(pszFilename);
        if (!mapDeviceToDriveLetter(&rc))
            rc.clear();
    }

    UnmapViewOfFile(pMem);
    CloseHandle(hFileMap);
    CloseHandle(hFile);
    return rc.isEmpty() ? f : rc;
}

// Make sure file can be found in editor manager and text markers
// Use '/', correct case and capitalize drive letter. Use a cache.

typedef QHash<QString, QString> NormalizedFileCache;
Q_GLOBAL_STATIC(NormalizedFileCache, normalizedFileNameCache)

QString CDBBreakPoint::normalizeFileName(const QString &f)
{
    QTC_ASSERT(!f.isEmpty(), return f)
    const NormalizedFileCache::const_iterator it = normalizedFileNameCache()->constFind(f);
    if (it != normalizedFileNameCache()->constEnd())
        return it.value();
    QString normalizedName = QDir::fromNativeSeparators(normalizeFileNameCaseHelper(f));
    // Upcase drive letter for consistency even if case mapping fails.
    if (normalizedName.size() > 2 && normalizedName.at(1) == QLatin1Char(':'))
        normalizedName[0] = normalizedName.at(0).toUpper();
    normalizedFileNameCache()->insert(f, normalizedName);
    return f;
}

void CDBBreakPoint::clearNormalizeFileNameCache()
{
    normalizedFileNameCache()->clear();
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

// Find a breakpoint by id
static inline QString msgNoBreakPointWithId(unsigned long id, const QString &why)
{
    return QString::fromLatin1("Unable to find breakpoint with id %1: %2").arg(id).arg(why);
}

static IDebugBreakpoint2 *breakPointById(CIDebugControl *ctl, unsigned long id, QString *errorMessage)
{
    CIDebugBreakpoint *ibp = 0;
    const HRESULT hr = ctl->GetBreakpointById2(id, &ibp);
    if (FAILED(hr)) {
        *errorMessage = msgNoBreakPointWithId(id, msgComFailed("GetBreakpointById2", hr));
        return 0;
    }
    if (!ibp) {
        *errorMessage = msgNoBreakPointWithId(id, QLatin1String("<not found>"));
        return 0;
    }
    return ibp;
}

// Remove breakpoint by id
static bool removeBreakPointById(CIDebugControl *ctl, unsigned long id, QString *errorMessage)
{
    if (debugBP)
        qDebug() << Q_FUNC_INFO << id;
    CIDebugBreakpoint *ibp = breakPointById(ctl, id, errorMessage);
    if (!ibp)
        return false;
    const HRESULT hr = ctl->RemoveBreakpoint2(ibp);
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Cannot remove breakpoint %1: %2").arg(id).arg(msgComFailed("RemoveBreakpointById2", hr));
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

static bool setBreakPointEnabledById(CIDebugControl *ctl, unsigned long id, bool enabled, QString *errorMessage)
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
        *errorMessage = msgCannotSetBreakPointEnabled(id, enabled, msgComFailed("GetFlags", hr));
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
        *errorMessage = msgCannotSetBreakPointEnabled(id, enabled, msgComFailed("SetFlags", hr));
        return false;
    }
    return true;
}

static inline QString msgCannotSetBreakAtFunction(const QString &func, const QString &why)
{
    return QString::fromLatin1("Cannot set a breakpoint at '%1': %2").arg(func, why);
}

// Synchronize (halted) engine breakpoints with those of the BreakHandler.
bool CDBBreakPoint::synchronizeBreakPoints(CIDebugControl* debugControl,
                                           CIDebugSymbols *syms,
                                           BreakHandler *handler,
                                           QString *errorMessage, QStringList *warnings)
{    
    errorMessage->clear();
    warnings->clear();
    // Do an initial check whether we are in a state that allows
    // for modifying  breakPoints
    ULONG engineCount;
    if (!getBreakPointCount(debugControl, &engineCount, errorMessage)) {
        *errorMessage = QString::fromLatin1("Cannot modify breakpoints: %1").arg(*errorMessage);
        return false;
    }
    QString warning;
    // Insert new ones
    bool updateMarkers = false;
    foreach (BreakpointData *nbd, handler->insertedBreakpoints()) {
        warning.clear();
        // Function breakpoints: Are the module names specified?
        bool breakPointOk = false;
        if (nbd->funcName.isEmpty()) {
            breakPointOk = true;
        } else {
            switch (resolveSymbol(syms, &nbd->funcName, &warning)) {
            case ResolveSymbolOk:
                breakPointOk = true;
                break;
            case ResolveSymbolAmbiguous:
                warnings->push_back(msgCannotSetBreakAtFunction(nbd->funcName, warning));
                warning.clear();
                breakPointOk = true;
                break;
            case ResolveSymbolNotFound:
            case ResolveSymbolError:
                warnings->push_back(msgCannotSetBreakAtFunction(nbd->funcName, warning));
                warning.clear();
                break;
            };
        } // function breakpoint
        // Now add...
        if (breakPointOk) {
            quint64 address;
            unsigned long id;
            CDBBreakPoint ncdbbp(*nbd);
            breakPointOk = ncdbbp.add(debugControl, &warning, &id, &address);
            if (breakPointOk) {
                if (debugBP)
                    qDebug() << "Added " << id << " at " << address << ncdbbp;
                handler->takeInsertedBreakPoint(nbd);
                updateMarkers = true;
                nbd->pending = false;
                nbd->bpNumber = QByteArray::number(uint(id));
                nbd->bpAddress = QLatin1String("0x") + QString::number(address, 16);
                // Take over rest as is
                nbd->bpCondition = nbd->condition;
                nbd->bpIgnoreCount = nbd->ignoreCount;
                nbd->bpFileName = nbd->fileName;
                nbd->bpLineNumber = nbd->lineNumber;
                nbd->bpFuncName = nbd->funcName;
            }
        } // had symbol
        if (!breakPointOk && !warning.isEmpty())
            warnings->push_back(warning);    }
    // Delete
    foreach (BreakpointData *rbd, handler->takeRemovedBreakpoints()) {
        if (!removeBreakPointById(debugControl, rbd->bpNumber.toUInt(), &warning))
            warnings->push_back(warning);
        delete rbd;
    }
    // Enable/Disable
    foreach (BreakpointData *ebd, handler->takeEnabledBreakpoints())
        if (!setBreakPointEnabledById(debugControl, ebd->bpNumber.toUInt(), true, &warning))
            warnings->push_back(warning);
    foreach (BreakpointData *dbd, handler->takeDisabledBreakpoints())
        if (!setBreakPointEnabledById(debugControl, dbd->bpNumber.toUInt(), false, &warning))
            warnings->push_back(warning);

    if (updateMarkers)
        handler->updateMarkers();

    if (debugBP > 1) {
        QList<CDBBreakPoint> bps;
        CDBBreakPoint::getBreakPoints(debugControl, &bps, errorMessage);
        qDebug().nospace() << "### Breakpoints in engine: " << bps;
    }
    return true;
}

} // namespace Internal
} // namespace Debugger
