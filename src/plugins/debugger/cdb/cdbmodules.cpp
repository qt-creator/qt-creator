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

#include "cdbmodules.h"
#include "moduleshandler.h"
#include "cdbdebugengine_p.h"

namespace Debugger {
namespace Internal {

bool getModuleList(IDebugSymbols3 *syms, QList<Module> *modules, QString *errorMessage)
{    
    modules->clear();
    ULONG loadedCount, unloadedCount;
    HRESULT hr = syms->GetNumberModules(&loadedCount, &unloadedCount);
    if (FAILED(hr)) {
        *errorMessage= msgComFailed("GetNumberModules", hr);
        return false;
    }
    // retrieve array of parameters
    const ULONG count = loadedCount + unloadedCount;
    QVector<DEBUG_MODULE_PARAMETERS> parameters(count);
    DEBUG_MODULE_PARAMETERS *parmPtr = &(*parameters.begin());
    memset(parmPtr, 0, sizeof(DEBUG_MODULE_PARAMETERS) * count);
    hr = syms->GetModuleParameters(count, 0, 0u, parmPtr);
    // E_INVALIDARG indicates 'Partial results' according to docu
    if (FAILED(hr) && hr != E_INVALIDARG) {
        *errorMessage= msgComFailed("GetModuleParameters", hr);
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
            hr = syms ->GetModuleNameStringWide(DEBUG_MODNAME_IMAGE, m, 0, wszBuf, MAX_PATH - 1, 0);
            if (FAILED(hr) && hr != E_INVALIDARG) {
                *errorMessage= msgComFailed("GetModuleNameStringWide", hr);
                return false;
            }
            module.moduleName = QString::fromUtf16(wszBuf);
            modules->push_back(module);
        }
    }
    return true;
}

// Search symbols matching a pattern
bool searchSymbols(IDebugSymbols3 *syms, const QString &pattern,
                   QStringList *matches, QString *errorMessage)
{
    matches->clear();    
    ULONG64 handle;
    // E_NOINTERFACE means "no match"
    HRESULT hr = syms->StartSymbolMatchWide(pattern.utf16(), &handle);
    if (hr == E_NOINTERFACE) {
        syms->EndSymbolMatch(handle);
        return true;
    }
    if (FAILED(hr)) {
        *errorMessage= msgComFailed("StartSymbolMatchWide", hr);
        return false;
    }
    WCHAR wszBuf[MAX_PATH];
    while (true) {
        hr = syms->GetNextSymbolMatchWide(handle, wszBuf, MAX_PATH - 1, 0, 0);
        if (hr == E_NOINTERFACE)
            break;
        if (hr == S_OK)
            matches->push_back(QString::fromUtf16(wszBuf));
    }
    syms->EndSymbolMatch(handle);
    if (matches->empty())
        *errorMessage = QString::fromLatin1("No symbol matches '%1'.").arg(pattern);
    return true;
}

// Add missing the module specifier: "main" -> "project!main"

ResolveSymbolResult resolveSymbol(IDebugSymbols3 *syms, QString *symbol,
                                  QString *errorMessage)
{
    // Is it an incomplete symbol?
    if (symbol->contains(QLatin1Char('!')))
        return ResolveSymbolOk;
    // 'main' is a #define for gdb, but not for VS
    if (*symbol == QLatin1String("qMain"))
        *symbol = QLatin1String("main");
    // resolve
    QStringList matches;
    if (!searchSymbols(syms, *symbol, &matches, errorMessage))
        return ResolveSymbolError;
    if (matches.empty())
        return ResolveSymbolNotFound;
    *symbol = matches.front();
    if (matches.size() > 1) {
        *errorMessage = QString::fromLatin1("Ambiguous symbol '%1': %2").
                        arg(*symbol, matches.join(QString(QLatin1Char(' '))));
        return ResolveSymbolAmbiguous;
    }
    return ResolveSymbolOk;
}

}
}
