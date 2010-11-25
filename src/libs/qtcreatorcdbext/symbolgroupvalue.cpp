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

#include "symbolgroupvalue.h"
#include "symbolgroup.h"
#include "stringutils.h"

SymbolGroupValueContext::SymbolGroupValueContext(CIDebugDataSpaces *ds)
    : dataspaces(ds)
{
}

SymbolGroupValueContext::SymbolGroupValueContext() : dataspaces(0)
{
}

SymbolGroupValue::SymbolGroupValue(SymbolGroupNode *node,
                                   const SymbolGroupValueContext &ctx) :
    m_node(node), m_context(ctx)
{
}

SymbolGroupValue::SymbolGroupValue() :
    m_node(0), m_errorMessage("Invalid")
{
}

bool SymbolGroupValue::isValid() const
{
    return m_node != 0 && m_context.dataspaces != 0;
}

SymbolGroupValue SymbolGroupValue::operator[](const char *name) const
{
    if (isValid() && m_node->expand(&m_errorMessage))
        if (SymbolGroupNode *child = m_node->childByIName(name))
            return SymbolGroupValue(child, m_context);
    return SymbolGroupValue();
}

std::string SymbolGroupValue::type() const
{
    return isValid() ? m_node->type() : std::string();
}

std::wstring SymbolGroupValue::value() const
{
    return isValid() ? m_node->fixedValue() : std::wstring();
}

int SymbolGroupValue::intValue(int defaultValue) const
{
    if (isValid()) {
        int rc = 0;
        if (integerFromString(wStringToString(value()), &rc))
            return rc;
    }
    return defaultValue;
}

ULONG64 SymbolGroupValue::pointerValue(ULONG64 defaultValue) const
{
    if (isValid()) {
        ULONG64 rc = 0;
        if (integerFromString(wStringToString(value()), &rc))
            return rc;
    }
    return defaultValue;
}

// Return allocated array of data
unsigned char *SymbolGroupValue::pointerData(unsigned length) const
{
    if (isValid()) {
        if (const ULONG64 ptr = pointerValue()) {
            unsigned char *data = new unsigned char[length];
            std::fill(data, data + length, 0);
            const HRESULT hr = m_context.dataspaces->ReadVirtual(ptr, data, length, NULL);
            if (FAILED(hr)) {
                delete [] data;
                return 0;
            }
            return data;
        }
    }
    return 0;
}

std::wstring SymbolGroupValue::wcharPointerData(unsigned charCount, unsigned maxCharCount) const
{
    const bool truncated = charCount > maxCharCount;
    if (truncated)
        charCount = maxCharCount;
    if (const unsigned char *ucData = pointerData(charCount * sizeof(wchar_t))) {
        const wchar_t *utf16Data = reinterpret_cast<const wchar_t *>(ucData);
        // Be smart about terminating 0-character
        if (charCount && utf16Data[charCount - 1] == 0)
            charCount--;
        std::wstring rc = std::wstring(utf16Data, charCount);
        if (truncated)
            rc += L"...";
        delete [] ucData;
        return rc;
    }
    return std::wstring();
}

std::string SymbolGroupValue::error() const
{
    return m_errorMessage;
}

// Dump a QString.
static bool dumpQString(const SymbolGroupValue &v, std::wstring *s)
{
    if (endsWith(v.type(), "QString")) {
        if (SymbolGroupValue d = v["d"]) {
            if (SymbolGroupValue sizeValue = d["size"]) {
                const int size = sizeValue.intValue();
                if (size >= 0) {
                    *s = d["data"].wcharPointerData(size);
                    return true;
                }
            }
        }
    }
    return false;
}

// Dump builtin simple types using SymbolGroupValue expressions.
bool dumpSimpleType(SymbolGroupNode  *n, const SymbolGroupValueContext &ctx, std::wstring *s)
{
    const SymbolGroupValue v(n, ctx);
    return dumpQString(v, s);
}
