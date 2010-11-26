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

#ifndef SYMBOLGROUPVALUE_H
#define SYMBOLGROUPVALUE_H

#include "common.h"

#include <string>

class SymbolGroupNode;

// Structure to pass all IDebug interfaces used for SymbolGroupValue
struct SymbolGroupValueContext
{
    SymbolGroupValueContext(CIDebugDataSpaces *ds) : dataspaces(ds) {}
    SymbolGroupValueContext::SymbolGroupValueContext() : dataspaces(0) {}

    CIDebugDataSpaces *dataspaces;
};

/* SymbolGroupValue: Flyweight tied to a SymbolGroupNode
 * providing a convenient operator[] + value getters for notation of dumpers.
 * Inaccesible members return a SymbolGroupValue in state 'invalid'. */

class SymbolGroupValue
{
public:
    explicit SymbolGroupValue(SymbolGroupNode *node, const SymbolGroupValueContext &c);
    SymbolGroupValue();

    operator bool() const { return isValid(); }
    bool isValid() const;

    // Access children by name or index (0-based)
    SymbolGroupValue operator[](const char *name) const;
    SymbolGroupValue operator[](unsigned) const;

    std::string type() const;
    std::wstring value() const;
    int intValue(int defaultValue = -1) const;
    double floatValue(double defaultValue = -999) const;
    ULONG64 pointerValue(ULONG64 defaultValue = 0) const;
    // Return allocated array of data pointed to
    unsigned char *pointerData(unsigned length) const;
    // Return data pointed to as wchar_t/std::wstring (UTF16)
    std::wstring wcharPointerData(unsigned charCount, unsigned maxCharCount = 512) const;

    std::string error() const;

private:
    bool ensureExpanded() const;

    SymbolGroupNode *m_node;
    SymbolGroupValueContext m_context;
    mutable std::string m_errorMessage;
};

// Dump builtin simple types using SymbolGroupValue expressions,
// returning SymbolGroupNode dumper flags.
unsigned dumpSimpleType(SymbolGroupNode  *n, const SymbolGroupValueContext &ctx, std::wstring *s);

#endif // SYMBOLGROUPVALUE_H
