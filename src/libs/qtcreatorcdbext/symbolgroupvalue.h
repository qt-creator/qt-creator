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

    std::string type() const;
    std::vector<std::string>  innerTypes() const { return innerTypesOf(type()); }
    std::wstring value() const;
    unsigned size() const;

    int intValue(int defaultValue = -1) const;
    double floatValue(double defaultValue = -999) const;
    ULONG64 pointerValue(ULONG64 defaultValue = 0) const;
    ULONG64 address() const;
    // Return allocated array of data pointed to
    unsigned char *pointerData(unsigned length) const;
    // Return data pointed to as wchar_t/std::wstring (UTF16)
    std::wstring wcharPointerData(unsigned charCount, unsigned maxCharCount = 512) const;

    std::string error() const;

    static inline unsigned sizeOf(const char *type) { return GetTypeSize(type); }
    static std::string stripPointerType(const std::string &);
    static std::string stripArrayType(const std::string &);
    // get the inner types: "QMap<int, double>" -> "int", "double"
    static std::vector<std::string> innerTypesOf(const std::string &t);

private:
    bool ensureExpanded() const;
    SymbolGroupValue typeCastedValue(ULONG64 address, const char *type) const;

    SymbolGroupNode *m_node;
    SymbolGroupValueContext m_context;
    mutable std::string m_errorMessage;
};

// Helpers for detecting types
enum KnownType {
    KT_Unknown =0,
    KT_Qt_Type = 0x10000,
    KT_STL_Type = 0x20000,
    KT_ContainerType = 0x40000,
    // Qt Basic
    KT_QChar = KT_Qt_Type + 1,
    KT_QByteArray = KT_Qt_Type + 2,
    KT_QString = KT_Qt_Type + 3,
    KT_QColor = KT_Qt_Type + 4,
    KT_QFlags = KT_Qt_Type + 5,
    KT_QDate = KT_Qt_Type + 6,
    KT_QTime = KT_Qt_Type + 7,
    KT_QPoint = KT_Qt_Type + 8,
    KT_QPointF = KT_Qt_Type + 9,
    KT_QSize = KT_Qt_Type + 11,
    KT_QSizeF = KT_Qt_Type + 12,
    KT_QLine = KT_Qt_Type + 13,
    KT_QLineF = KT_Qt_Type + 14,
    KT_QRect = KT_Qt_Type + 15,
    KT_QRectF = KT_Qt_Type + 16,
    KT_QVariant = KT_Qt_Type + 17,
    KT_QBasicAtomicInt = KT_Qt_Type + 18,
    KT_QAtomicInt = KT_Qt_Type + 19,
    KT_QObject = KT_Qt_Type + 20,
    KT_QWidget = KT_Qt_Type + 21,
    // Qt Containers
    KT_QStringList = KT_Qt_Type + KT_ContainerType + 1,
    KT_QList = KT_Qt_Type + KT_ContainerType + 2,
    KT_QVector = KT_Qt_Type + KT_ContainerType + 3,
    KT_QSet = KT_Qt_Type + KT_ContainerType + 4,
    KT_QHash = KT_Qt_Type + KT_ContainerType + 5,
    KT_QMap = KT_Qt_Type + KT_ContainerType + 6,
    KT_QMultiMap = KT_Qt_Type + KT_ContainerType + 7,
    // STL
    KT_StdString = KT_STL_Type + 1,
    KT_StdWString = KT_STL_Type + 2,
    // STL containers
    KT_StdVector =  KT_STL_Type + KT_ContainerType + 1,
    KT_StdList =  KT_STL_Type + KT_ContainerType + 2,
    KT_StdSet =  KT_STL_Type + KT_ContainerType + 3,
    KT_StdMap =  KT_STL_Type + KT_ContainerType + 4,
    KT_StdMultiMap =  KT_STL_Type + KT_ContainerType + 5,
};

KnownType knownType(const std::string &type);

// Dump builtin simple types using SymbolGroupValue expressions,
// returning SymbolGroupNode dumper flags.
unsigned dumpSimpleType(SymbolGroupNode  *n, const SymbolGroupValueContext &ctx,
                        std::wstring *s,
                        int *knownType = 0,
                        int *containerSizeIn = 0);

// Return size of container or -1
int containerSize(KnownType ct, const SymbolGroupValue &v);
int containerSize(KnownType ct, SymbolGroupNode  *n, const SymbolGroupValueContext &ctx);

#endif // SYMBOLGROUPVALUE_H
