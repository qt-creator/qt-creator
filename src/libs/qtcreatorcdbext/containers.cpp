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

#include "containers.h"
#include "symbolgroupvalue.h"
#include "symbolgroup.h"
#include "stringutils.h"

typedef SymbolGroupNode::SymbolGroupNodePtrVector SymbolGroupNodePtrVector;

/* Helper for array-type containers:
 * Add a series of "*(innertype *)0x (address + n * size)" fake child symbols. */
static SymbolGroupNodePtrVector arrayChildList(SymbolGroup *sg, ULONG64 address,
                                               int count, const std::string &innerType)
{
    SymbolGroupNodePtrVector rc;
    const unsigned innerTypeSize = SymbolGroupValue::sizeOf(innerType.c_str());
    if (!innerTypeSize)
        return rc;

    std::string errorMessage;
    rc.reserve(count);
    for (int i = 0; i < count; i++, address += innerTypeSize) {
        std::ostringstream str;
        str << "*(" << innerType << " *)" << std::showbase << std::hex << address;
        if (SymbolGroupNode *child = sg->addSymbol(str.str(), toString(i), &errorMessage)) {
            rc.push_back(child);
        } else {
            break;
        }
    }
    return rc;
}

// std::vector<T>
static inline SymbolGroupNodePtrVector
    stdVectorChildList(SymbolGroupNode *n, int count, const SymbolGroupValueContext &ctx)
{
    if (count) {
        // std::vector<T>: _Myfirst is a pointer of T*. Get address
        // element to obtain address.
        const SymbolGroupValue vec(n, ctx);
        SymbolGroupValue myFirst = vec[unsigned(0)]["_Myfirst"]; // MSVC2010
        if (!myFirst)
            myFirst = vec["_Myfirst"]; // MSVC2008
        if (myFirst)
            if (const ULONG64 address = myFirst.pointerValue())
                return arrayChildList(n->symbolGroup(), address, count,
                                      SymbolGroupValue::stripPointerType(myFirst.type()));
    }
    return SymbolGroupNodePtrVector();
}

// QVector<T>
static inline SymbolGroupNodePtrVector
    qVectorChildList(SymbolGroupNode *n, int count, const SymbolGroupValueContext &ctx)
{
    if (count) {
        // QVector<T>: p/array is declared as array of T. Dereference first
        // element to obtain address.
        const SymbolGroupValue vec(n, ctx);
        if (const SymbolGroupValue firstElementV = vec["p"]["array"][unsigned(0)])
            if (const ULONG64 arrayAddress = firstElementV.address())
                return arrayChildList(n->symbolGroup(), arrayAddress, count,
                                      firstElementV.type());
    }
    return SymbolGroupNodePtrVector();
}

// QList<> of type array
static inline SymbolGroupNodePtrVector
    qListOfArraryTypeChildren(SymbolGroup *sg, const SymbolGroupValue &v, int count)
{
    // QList<T>: d/array is declared as array of void *[]. Dereference first
    // element to obtain address.
    if (count) {
        if (const SymbolGroupValue firstElementV = v["d"]["array"][unsigned(0)])
            if (const ULONG64 arrayAddress = firstElementV.address()) {
                const std::vector<std::string> innerTypes = v.innerTypes();
                if (innerTypes.size() == 1)
                    return arrayChildList(sg, arrayAddress,
                                          count, innerTypes.front());
            }
    }
    return SymbolGroupNodePtrVector();
}

SymbolGroupNodePtrVector containerChildren(SymbolGroupNode *node, int type,
                                           int size, const SymbolGroupValueContext &ctx)
{
    if (!size)
        return SymbolGroupNodePtrVector();
    if (size > 100)
        size = 100;
    switch (type) {
    case KT_QVector:
        return qVectorChildList(node, size, ctx);
    case KT_StdVector:
        return stdVectorChildList(node, size, ctx);
    case KT_QList:
        // Differentiate between array and list
        break;
    case KT_QStringList:
        if (const SymbolGroupValue qList = SymbolGroupValue(node, ctx)[unsigned(0)])
            return qListOfArraryTypeChildren(node->symbolGroup(), qList, size);
        break;
    }
    return SymbolGroupNodePtrVector();
}
