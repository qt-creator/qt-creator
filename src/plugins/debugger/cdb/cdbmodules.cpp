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

#include "cdbmodules.h"
#include "moduleshandler.h"
#include "cdbdebugengine_p.h"

#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>

namespace Debugger {
namespace Internal {

static inline bool getModuleCount(CIDebugSymbols *syms, ULONG *count, QString *errorMessage)
{
    *count = 0;
    ULONG loadedCount, unloadedCount;
    const HRESULT hr = syms->GetNumberModules(&loadedCount, &unloadedCount);
    if (FAILED(hr)) {
        *errorMessage= CdbCore::msgComFailed("GetNumberModules", hr);
        return false;
    }
    *count = loadedCount + unloadedCount;
    return true;
}

bool getModuleNameList(CIDebugSymbols *syms, QStringList *modules, QString *errorMessage)
{
    ULONG count;
    modules->clear();
    if (!getModuleCount(syms, &count, errorMessage))
        return false;
    WCHAR wszBuf[MAX_PATH];
    for (ULONG m = 0; m < count; m++)
        if (SUCCEEDED(syms->GetModuleNameStringWide(DEBUG_MODNAME_IMAGE, m, 0, wszBuf, MAX_PATH - 1, 0)))
                modules->push_back(QString::fromUtf16(reinterpret_cast<const ushort *>(wszBuf)));
    return true;
}

bool getModuleList(CIDebugSymbols *syms, QList<Module> *modules, QString *errorMessage)
{    
    ULONG count;
    modules->clear();
    if (!getModuleCount(syms, &count, errorMessage))
        return false;
    QVector<DEBUG_MODULE_PARAMETERS> parameters(count);
    DEBUG_MODULE_PARAMETERS *parmPtr = &(*parameters.begin());
    memset(parmPtr, 0, sizeof(DEBUG_MODULE_PARAMETERS) * count);
    HRESULT hr = syms->GetModuleParameters(count, 0, 0u, parmPtr);
    // E_INVALIDARG indicates 'Partial results' according to docu
    if (FAILED(hr) && hr != E_INVALIDARG) {
        *errorMessage= CdbCore::msgComFailed("GetModuleParameters", hr);
        return false;
    }
    // fill array
    const QString hexPrefix = QLatin1String("0x");
    WCHAR wszBuf[MAX_PATH];
    for (ULONG m = 0; m < count; m++) {
        const DEBUG_MODULE_PARAMETERS &p = parameters.at(m);
        if (p.Base != DEBUG_INVALID_OFFSET) { // Partial results?
            Module module;
            module.symbolsRead = (p.Flags & DEBUG_MODULE_USER_MODE)
                            && (p.SymbolType != DEBUG_SYMTYPE_NONE);
            module.startAddress = hexPrefix + QString::number(p.Base, 16);
            module.endAddress = hexPrefix + QString::number((p.Base + p.Size), 16);
            hr = syms->GetModuleNameStringWide(DEBUG_MODNAME_IMAGE, m, 0, wszBuf, MAX_PATH - 1, 0);
            if (FAILED(hr) && hr != E_INVALIDARG) {
                *errorMessage= CdbCore::msgComFailed("GetModuleNameStringWide", hr);
                return false;
            }
            module.moduleName = QString::fromUtf16(reinterpret_cast<const ushort *>(wszBuf));
            modules->push_back(module);
        }
    }
    return true;
}

// Search symbols matching a pattern
bool searchSymbols(CIDebugSymbols *syms, const QString &pattern,
                   QStringList *matches, QString *errorMessage)
{
    matches->clear();    
    ULONG64 handle = 0;
    // E_NOINTERFACE means "no match". Apparently, it does not always
    // set handle.
    HRESULT hr = syms->StartSymbolMatchWide(reinterpret_cast<PCWSTR>(pattern.utf16()), &handle);
    if (hr == E_NOINTERFACE) {
        if (handle)
            syms->EndSymbolMatch(handle);
        return true;
    }
    if (FAILED(hr)) {
        *errorMessage= CdbCore::msgComFailed("StartSymbolMatchWide", hr);
        return false;
    }
    WCHAR wszBuf[MAX_PATH];
    while (true) {
        hr = syms->GetNextSymbolMatchWide(handle, wszBuf, MAX_PATH - 1, 0, 0);
        if (hr == E_NOINTERFACE)
            break;
        if (hr == S_OK)
            matches->push_back(QString::fromUtf16(reinterpret_cast<const ushort *>(wszBuf)));
    }
    syms->EndSymbolMatch(handle);
    if (matches->empty())
        *errorMessage = QString::fromLatin1("No symbol matches '%1'.").arg(pattern);
    return true;
}

// Helper for the resolveSymbol overloads.
static ResolveSymbolResult resolveSymbol(CIDebugSymbols *syms, QString *symbol,
                                         QStringList *matches,
                                         QString *errorMessage)
{
    errorMessage->clear();
    // Is it an incomplete symbol?
    if (symbol->contains(QLatin1Char('!')))
        return ResolveSymbolOk;
    // 'main' is a #define for gdb, but not for VS
    if (*symbol == QLatin1String("qMain"))
        *symbol = QLatin1String("main");
    // resolve
    if (!searchSymbols(syms, *symbol, matches, errorMessage))
        return ResolveSymbolError;
    if (matches->empty())
        return ResolveSymbolNotFound;
    *symbol = matches->front();
    if (matches->size() > 1) {
        *errorMessage = QString::fromLatin1("Ambiguous symbol '%1': %2").
                        arg(*symbol, matches->join(QString(QLatin1Char(' '))));
        return ResolveSymbolAmbiguous;
    }
    return ResolveSymbolOk;
}

// Add missing the module specifier: "main" -> "project!main"
ResolveSymbolResult resolveSymbol(CIDebugSymbols *syms, QString *symbol,
                                  QString *errorMessage)
{
    QStringList matches;
    return resolveSymbol(syms, symbol, &matches, errorMessage);
}

ResolveSymbolResult resolveSymbol(CIDebugSymbols *syms, const QString &pattern, QString *symbol, QString *errorMessage)
{
    QStringList matches;
    const ResolveSymbolResult r1 = resolveSymbol(syms, symbol, &matches, errorMessage);
    switch (r1) {
    case ResolveSymbolOk:
    case ResolveSymbolNotFound:
    case ResolveSymbolError:
        return r1;
    case ResolveSymbolAmbiguous:
        break;
    }
    // Filter out
    errorMessage->clear();
    const QRegExp re(pattern);
    if (!re.isValid()) {
        *errorMessage = QString::fromLatin1("Internal error: Invalid pattern '%1'.").arg(pattern);
        return ResolveSymbolError;
    }
    const QStringList filteredMatches = matches.filter(re);
    if (filteredMatches.size() == 1) {
        *symbol = filteredMatches.front();
        return ResolveSymbolOk;
    }
    // something went wrong
    const QString matchesString = matches.join(QString(QLatin1Char(',')));
    if (filteredMatches.empty()) {
        *errorMessage = QString::fromLatin1("None of symbols '%1' found for '%2' matches '%3'.").
                        arg(matchesString, *symbol, pattern);
        return ResolveSymbolNotFound;
    }
    *errorMessage = QString::fromLatin1("Ambiguous match of symbols '%1' found for '%2' (%3)").
                        arg(matchesString, *symbol, pattern);
    return ResolveSymbolAmbiguous;
}

// List symbols of a module
bool getModuleSymbols(CIDebugSymbols *syms, const QString &moduleName,
                      QList<Symbol> *symbols, QString *errorMessage)
{
    // Search all symbols and retrieve addresses
    symbols->clear();
    QStringList matches;
    const QString pattern = QFileInfo(moduleName).baseName() + QLatin1String("!*");
    if (!searchSymbols(syms, pattern, &matches, errorMessage))
        return false;
    const QString hexPrefix = QLatin1String("0x");
    foreach (const QString &name, matches) {
        Symbol symbol;
        symbol.name = name;
        ULONG64 offset = 0;
        if (SUCCEEDED(syms->GetOffsetByNameWide(reinterpret_cast<PCWSTR>(name.utf16()), &offset)))
            symbol.address = hexPrefix + QString::number(offset, 16);
        symbols->push_back(symbol);
    }
    return true;
}

} // namespace Internal
} // namespace Debugger
