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

#ifndef SYMBOLGROUPVALUE_H
#define SYMBOLGROUPVALUE_H

#include "common.h"
#include "knowntype.h"

#include <string>
#include <vector>

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
    // take address and cast to desired (pointer) type
    SymbolGroupValue typeCast(const char *type) const;
    // take pointer value and cast to desired (pointer) type
    SymbolGroupValue pointerTypeCast(const char *type) const;

    std::string name() const;
    std::string type() const;
    std::vector<std::string>  innerTypes() const { return innerTypesOf(type()); }
    std::wstring value() const;
    unsigned size() const;

    SymbolGroupNode *node() const { return m_node; }
    SymbolGroupValueContext context() const { return m_context; }

    int intValue(int defaultValue = -1) const;
    double floatValue(double defaultValue = -999) const;
    ULONG64 pointerValue(ULONG64 defaultValue = 0) const;
    ULONG64 address() const;
    // Return allocated array of data pointed to
    unsigned char *pointerData(unsigned length) const;
    // Return data pointed to as wchar_t/std::wstring (UTF16)
    std::wstring wcharPointerData(unsigned charCount, unsigned maxCharCount = 512) const;

    std::string error() const;

    // Some helpers for manipulating types.
    static inline unsigned sizeOf(const char *type) { return GetTypeSize(type); }
    static std::string stripPointerType(const std::string &);
    static std::string addPointerType(const std::string &);
    static std::string stripArrayType(const std::string &);
    static bool isPointerType(const std::string &);
    static unsigned pointerSize();

    // get the inner types: "QMap<int, double>" -> "int", "double"
    static std::vector<std::string> innerTypesOf(const std::string &t);

private:
    bool ensureExpanded() const;
    SymbolGroupValue typeCastedValue(ULONG64 address, const char *type) const;

    SymbolGroupNode *m_node;
    SymbolGroupValueContext m_context;
    mutable std::string m_errorMessage;
};

// For debugging purposes
std::ostream &operator<<(std::ostream &, const SymbolGroupValue &v);

/* Helpers for detecting types reported from IDebugSymbolGroup
 * 1) Class prefix==true is applicable to outer types obtained from
 *    from IDebugSymbolGroup: 'class foo' or 'struct foo'.
 * 2) Class prefix==false is for inner types of templates, etc, doing
 *    a more expensive check:  'foo' */
KnownType knownType(const std::string &type, bool hasClassPrefix = true);

// Dump builtin simple types using SymbolGroupValue expressions,
// returning SymbolGroupNode dumper flags.
unsigned dumpSimpleType(SymbolGroupNode  *n, const SymbolGroupValueContext &ctx,
                        std::wstring *s,
                        int *knownType = 0,
                        int *containerSizeIn = 0);

#endif // SYMBOLGROUPVALUE_H
