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

#include "cdbsymbolgroupcontext.h"
#include "cdbdebugengine_p.h"
#include "watchhandler.h"

// A helper function to extract a string value from a member function of
// IDebugSymbolGroup2 taking the symbol index and a character buffer.
// Pass in the the member function as '&IDebugSymbolGroup2::GetSymbolNameWide'

typedef HRESULT  (__stdcall IDebugSymbolGroup2::*WideStringRetrievalFunction)(ULONG, PWSTR, ULONG, PULONG);

static inline QString getSymbolString(IDebugSymbolGroup2 *sg,
                                      WideStringRetrievalFunction wsf,
                                      unsigned long index)
{
    static WCHAR nameBuffer[MAX_PATH + 1];
    // Name
    ULONG nameLength;
    const HRESULT hr = (sg->*wsf)(index, nameBuffer, MAX_PATH, &nameLength);
    if (SUCCEEDED(hr)) {
        nameBuffer[nameLength] = 0;
        return QString::fromUtf16(nameBuffer);
    }
    return QString();
}

namespace Debugger {
    namespace Internal {

CdbSymbolGroupContext::CdbSymbolGroupContext(const QString &prefix,
                                             IDebugSymbolGroup2 *symbolGroup) :
    m_prefix(prefix),
    m_nameDelimiter(QLatin1Char('.')),
    m_symbolGroup(symbolGroup)
{
}

CdbSymbolGroupContext::~CdbSymbolGroupContext()
{
    m_symbolGroup->Release();
}

CdbSymbolGroupContext::Range
    CdbSymbolGroupContext::getSymbolRange(const QString &prefix)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << prefix;
    const ChildRangeMap::const_iterator it = m_childRanges.constFind(prefix);
    if (it != m_childRanges.constEnd())
        return it.value();
    const Range r = prefix == m_prefix ? allocateRootSymbols() : allocateChildSymbols(prefix);
    m_childRanges.insert(prefix, r);
    return r;
}

CdbSymbolGroupContext::Range
    CdbSymbolGroupContext::allocateChildSymbols(const QString &prefix)
{
    unsigned long startPos = 0;
    unsigned long count = 0;

    bool success = false;
    QString errorMessage;
    do {
        const int parentIndex = m_symbolINames.indexOf(prefix);
        if (parentIndex == -1) {
            errorMessage = QString::fromLatin1("Prefix not found '%1'").arg(prefix);
            break;
        }

        success = true;
    } while (false);
    if (!success) {
        qWarning("%s\n", qPrintable(errorMessage));
    }
    return Range(startPos, count);
}

CdbSymbolGroupContext::Range
    CdbSymbolGroupContext::allocateRootSymbols()
{
    unsigned long startPos = 0;
    unsigned long count = 0;
    bool success = false;

    QString errorMessage;
    do {
        HRESULT hr = m_symbolGroup->GetNumberSymbols(&count);
        if (FAILED(hr)) {
            errorMessage = msgComFailed("GetNumberSymbols", hr);
            break;
        }

        m_symbolParameters.reserve(3u * count);
        m_symbolParameters.resize(count);

        hr = m_symbolGroup->GetSymbolParameters(0, count, symbolParameters());
        if (FAILED(hr)) {
            errorMessage = msgComFailed("GetSymbolParameters", hr);
            break;
        }
        const QString symbolPrefix = m_prefix + m_nameDelimiter;
        for (unsigned long i = 0; i < count; i++)
            m_symbolINames.push_back(symbolPrefix + getSymbolString(m_symbolGroup, &IDebugSymbolGroup2::GetSymbolNameWide, i));

        success = true;
    } while (false);
    if (!success) {
        clear();
        count = 0;
        qWarning("%s\n", qPrintable(errorMessage));
    }
    return Range(startPos, count);
}

void CdbSymbolGroupContext::clear()
{
    m_symbolParameters.clear();
    m_childRanges.clear();
    m_symbolINames.clear();
}

WatchData CdbSymbolGroupContext::symbolAt(unsigned long index) const
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << index;

    WatchData wd;
    wd.iname = m_symbolINames.at(index);
    const int lastDelimiterPos = wd.iname.lastIndexOf(m_nameDelimiter);
    wd.name = lastDelimiterPos == -1 ? wd.iname : wd.iname.mid(lastDelimiterPos + 1);
    wd.type = getSymbolString(m_symbolGroup, &IDebugSymbolGroup2::GetSymbolTypeNameWide, index);
    wd.value = getSymbolString(m_symbolGroup, &IDebugSymbolGroup2::GetSymbolValueTextWide, index);
    const DEBUG_SYMBOL_PARAMETERS &params = m_symbolParameters.at(index);
    if (params.SubElements) {
        wd.setTypeUnneeded();
        wd.setValueUnneeded();
        wd.setChildCount(1);
    } else {
        wd.setAllUnneeded();
    }
    if (debugCDB) {
        qDebug() << Q_FUNC_INFO << wd.toString();
    }
    return wd;
}

}
}
