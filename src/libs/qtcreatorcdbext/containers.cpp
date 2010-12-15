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

#include <functional>

typedef AbstractSymbolGroupNode::AbstractSymbolGroupNodePtrVector AbstractSymbolGroupNodePtrVector;

// Return size of container or -1
int containerSize(KnownType kt, SymbolGroupNode *n, const SymbolGroupValueContext &ctx)
{
    QTC_TRACE_IN
    if ((kt & KT_ContainerType) == 0)
        return -1;
    const int ct = containerSize(kt, SymbolGroupValue(n, ctx));
    QTC_TRACE_OUT
    return ct;
}

// Return size from an STL vector (last/first iterators).
static inline int msvcStdVectorSize(const SymbolGroupValue &v)
{
    if (const SymbolGroupValue myFirstPtrV = v["_Myfirst"]) {
        if (const SymbolGroupValue myLastPtrV = v["_Mylast"]) {
            const ULONG64 firstPtr = myFirstPtrV.pointerValue();
            const ULONG64 lastPtr = myLastPtrV.pointerValue();
            if (!firstPtr || lastPtr < firstPtr)
                return -1;
            if (lastPtr == firstPtr)
                return 0;
            // Subtract the pointers: We need to do the pointer arithmetics ourselves
            // as we get char *pointers.
            const std::string innerType = SymbolGroupValue::stripPointerType(myFirstPtrV.type());
            const size_t size = SymbolGroupValue::sizeOf(innerType.c_str());
            if (size == 0)
                return -1;
            return static_cast<int>((lastPtr - firstPtr) / size);
        }
    }
    return -1;
}

// Determine size of containers
int containerSize(KnownType kt, const SymbolGroupValue &v)
{
    switch (kt) {
    case KT_QStringList:
        if (const SymbolGroupValue base = v[unsigned(0)])
            return containerSize(KT_QList, base);
        break;
    case KT_QList:
        if (const SymbolGroupValue dV = v["d"]) {
            if (const SymbolGroupValue beginV = dV["begin"]) {
                const int begin = beginV.intValue();
                const int end = dV["end"].intValue();
                if (begin >= 0 && end >= begin)
                    return end - begin;
            }
        }
        break;
    case KT_QLinkedList:
    case KT_QHash:
    case KT_QMap:
    case KT_QVector:
        if (const SymbolGroupValue sizeV = v["d"]["size"])
            return sizeV.intValue();
        break;
    case KT_QQueue:
        if (const SymbolGroupValue qList= v[unsigned(0)])
            return containerSize(KT_QList, qList);
        break;
    case KT_QStack:
        if (const SymbolGroupValue qVector = v[unsigned(0)])
            return containerSize(KT_QVector, qVector);
        break;
    case KT_QSet:
        if (const SymbolGroupValue base = v[unsigned(0)])
            return containerSize(KT_QHash, base);
        break;
    case KT_QMultiMap:
        if (const SymbolGroupValue base = v[unsigned(0)])
            return containerSize(KT_QMap, base);
        break;
    case KT_StdVector: {
        if (const SymbolGroupValue base = v[unsigned(0)]) {
            const int msvc10Size = msvcStdVectorSize(base);
            if (msvc10Size >= 0)
                return msvc10Size;
        }
        const int msvc8Size = msvcStdVectorSize(v);
        if (msvc8Size >= 0)
            return msvc8Size;
    }
        break;
    case KT_StdList:
        if (const SymbolGroupValue sizeV =  v["_Mysize"]) // VS 8
            return sizeV.intValue();
        if (const SymbolGroupValue sizeV = v[unsigned(0)][unsigned(0)]["_Mysize"]) // VS10
            return sizeV.intValue();
        break;
    case KT_StdDeque:
        if (const SymbolGroupValue sizeV =  v[unsigned(0)]["_Mysize"])
            return sizeV.intValue();
        break;
    case KT_StdStack:
        if (const SymbolGroupValue deque =  v[unsigned(0)])
            return containerSize(KT_StdDeque, deque);
        break;
    case KT_StdSet:
    case KT_StdMap:
    case KT_StdMultiMap:
        if (const SymbolGroupValue baseV = v[unsigned(0)]) {
            if (const SymbolGroupValue sizeV = baseV["_Mysize"]) // VS 8
                return sizeV.intValue();
            if (const SymbolGroupValue sizeV = baseV[unsigned(0)][unsigned(0)]["_Mysize"]) // VS 10
                return sizeV.intValue();
        }
        break;
    }
    return -1;
}

/* Generate a list of children by invoking the functions to obtain the value
 * and the next link */
template <class ValueFunction, class NextFunction>
AbstractSymbolGroupNodePtrVector linkedListChildList(SymbolGroupValue headNode,
                                                     int count,
                                                     ValueFunction valueFunc,
                                                     NextFunction nextFunc)
{
    AbstractSymbolGroupNodePtrVector rc;
    rc.reserve(count);
    for (int i =0; i < count && headNode; i++) {
        if (const SymbolGroupValue value = valueFunc(headNode)) {
            rc.push_back(ReferenceSymbolGroupNode::createArrayNode(i, value.node()));
            headNode = nextFunc(headNode);
        } else {
            break;
        }
    }
    return rc;
}

// Helper function for linkedListChildList that returns a member by name
class MemberByName : public std::unary_function<const SymbolGroupValue &, SymbolGroupValue>
{
public:
    explicit MemberByName(const char *name) : m_name(name) {}
    SymbolGroupValue operator()(const SymbolGroupValue &v) { return v[m_name]; }

private:
    const char *m_name;
};

// std::list<T>: Dummy head node and then a linked list of "_Next", "_Myval".
static inline AbstractSymbolGroupNodePtrVector stdListChildList(SymbolGroupNode *n, int count,
                                                        const SymbolGroupValueContext &ctx)
{
    if (count)
        if (const SymbolGroupValue head = SymbolGroupValue(n, ctx)[unsigned(0)][unsigned(0)]["_Myhead"]["_Next"])
            return linkedListChildList(head, count, MemberByName("_Myval"), MemberByName("_Next"));
    return AbstractSymbolGroupNodePtrVector();
}

// QLinkedList<T>: Dummy head node and then a linked list of "n", "t".
static inline AbstractSymbolGroupNodePtrVector qLinkedListChildList(SymbolGroupNode *n, int count,
                                                        const SymbolGroupValueContext &ctx)
{
    if (count)
        if (const SymbolGroupValue head = SymbolGroupValue(n, ctx)["e"]["n"])
            return linkedListChildList(head, count, MemberByName("t"), MemberByName("n"));
    return AbstractSymbolGroupNodePtrVector();
}

/* Helper for array-type containers:
 * Add a series of "*(innertype *)0x (address + n * size)" fake child symbols.
 * for a function generating a sequence of addresses. */
template <class AddressFunc>
AbstractSymbolGroupNodePtrVector arrayChildList(SymbolGroup *sg, AddressFunc addressFunc,
                                        const std::string &innerType, int count)
{
    AbstractSymbolGroupNodePtrVector rc;
    if (!count)
        return rc;
    std::string errorMessage;
    rc.reserve(count);
    for (int i = 0; i < count; i++) {
        std::ostringstream str;
        str << "*(" << innerType;
        if (!endsWith(innerType, '*'))
            str << ' ';
        str << "*)" << std::showbase << std::hex << addressFunc();
        if (SymbolGroupNode *child = sg->addSymbol(str.str(), std::string(), &errorMessage)) {
            rc.push_back(ReferenceSymbolGroupNode::createArrayNode(i, child));
        } else {
            break;
        }
    }
    return rc;
}

// Helper function for arrayChildList() taking a reference to an address and simply generating
// a sequence of address, address + delta, address + 2 * delta...
class AddressSequence
{
public:
    explicit inline AddressSequence(ULONG64 &address, ULONG delta) : m_address(address), m_delta(delta) {}
    inline ULONG64 operator()()
    {
        const ULONG64 rc = m_address;
        m_address += m_delta;
        return rc;
    }

private:
    ULONG64 &m_address;
    const ULONG m_delta;
};

static inline AbstractSymbolGroupNodePtrVector arrayChildList(SymbolGroup *sg, ULONG64 address,
                                               const std::string &innerType, int count)
{
    if (const unsigned innerTypeSize = SymbolGroupValue::sizeOf(innerType.c_str()))
        return arrayChildList(sg, AddressSequence(address, innerTypeSize),
                              innerType, count);
    return AbstractSymbolGroupNodePtrVector();
}

// std::vector<T>
static inline AbstractSymbolGroupNodePtrVector
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
                return arrayChildList(n->symbolGroup(), address,
                                      SymbolGroupValue::stripPointerType(myFirst.type()), count);
    }
    return AbstractSymbolGroupNodePtrVector();
}

// QVector<T>
static inline AbstractSymbolGroupNodePtrVector
    qVectorChildList(SymbolGroupNode *n, int count, const SymbolGroupValueContext &ctx)
{
    if (count) {
        // QVector<T>: p/array is declared as array of T. Dereference first
        // element to obtain address.
        const SymbolGroupValue vec(n, ctx);
        if (const SymbolGroupValue firstElementV = vec["p"]["array"][unsigned(0)])
            if (const ULONG64 arrayAddress = firstElementV.address())
                    return arrayChildList(n->symbolGroup(), arrayAddress, firstElementV.type(), count);
    }
    return AbstractSymbolGroupNodePtrVector();
}

// Helper function for arrayChildList() for use with QLists of large types that are an
// array of pointers to allocated elements: Generate a pointer sequence by reading out the array.
template <class AddressType>
class AddressArraySequence
{
public:
    explicit inline AddressArraySequence(const AddressType *array) : m_array(array) {}
    inline ULONG64 operator()() { return *m_array++; }

private:
    const AddressType *m_array;
};

// QList<>.
static inline AbstractSymbolGroupNodePtrVector
    qListChildList(const SymbolGroupValue &v, int count)
{
    // QList<T>: d/array is declared as array of void *[]. Dereference first
    // element to obtain address.
    if (!count)
        return AbstractSymbolGroupNodePtrVector();
    const SymbolGroupValue dV = v["d"];
    if (!dV)
        return AbstractSymbolGroupNodePtrVector();
    const int begin = dV["begin"].intValue();
    if (begin < 0)
        return AbstractSymbolGroupNodePtrVector();
    const SymbolGroupValue firstElementV = dV["array"][unsigned(0)];
    if (!firstElementV)
        return AbstractSymbolGroupNodePtrVector();
     ULONG64 arrayAddress = firstElementV.address();
     if (!arrayAddress)
         return AbstractSymbolGroupNodePtrVector();
     const std::vector<std::string> innerTypes = v.innerTypes();
     if (innerTypes.size() != 1)
         return AbstractSymbolGroupNodePtrVector();
     const std::string &innerType = innerTypes.front();
     const unsigned innerTypeSize = SymbolGroupValue::sizeOf(innerType.c_str());
     if (!innerTypeSize)
         return AbstractSymbolGroupNodePtrVector();
     /* QList<> is:
      * 1) An array of 'void *[]' where T values are coerced into the elements for
      *    POD/pointer types and small, movable or primitive Qt types. That is, smaller
      *    elements are also aligned at 'void *' boundaries.
      * 2) An array of 'T *[]' (pointer to allocated instances) for anything else
      *    (QTypeInfo<T>::isLarge || QTypeInfo<T>::isStatic)
      *    isStatic depends on QTypeInfo specializations and hardcoded flags for types. */
     const unsigned pointerSize = SymbolGroupValue::pointerSize();
     arrayAddress += begin * pointerSize;
     if (SymbolGroupValue::isPointerType(innerType)) // Quick check: Any pointer is T[]
         return arrayChildList(v.node()->symbolGroup(),
                               AddressSequence(arrayAddress, pointerSize),
                               innerType, count);
     // Check condition for large||static.
     bool isLargeOrStatic = innerTypeSize > pointerSize;
     if (!isLargeOrStatic) {
         const KnownType kt = knownType(innerType, false); // inner type, no 'class ' prefix.
         if (kt != KT_Unknown && !(knownType(innerType, false) & (KT_Qt_PrimitiveType|KT_Qt_MovableType)))
             isLargeOrStatic = true;
     }
     if (isLargeOrStatic) {
         // Retrieve the pointer array ourselves to avoid having to evaluate '*(class foo**)'
         const ULONG allocSize = pointerSize * count;
         ULONG bytesRead = 0;
         void *data = new unsigned char[pointerSize * count];
         const HRESULT hr = v.context().dataspaces->ReadVirtual(arrayAddress + begin * pointerSize, data, allocSize, &bytesRead);
         if (FAILED(hr) || bytesRead != allocSize) {
             delete [] data;
             return AbstractSymbolGroupNodePtrVector();
         }
         // Generate sequence of addresses from pointer array
         const AbstractSymbolGroupNodePtrVector rc = pointerSize == 8 ?
                     arrayChildList(v.node()->symbolGroup(), AddressArraySequence<ULONG64>(reinterpret_cast<const ULONG64 *>(data)), innerType, count) :
                     arrayChildList(v.node()->symbolGroup(), AddressArraySequence<ULONG32>(reinterpret_cast<const ULONG32 *>(data)), innerType, count);
         delete [] data;
         return rc;
     }
     return arrayChildList(v.node()->symbolGroup(),
                           AddressSequence(arrayAddress, pointerSize),
                           innerType, count);
}

AbstractSymbolGroupNodePtrVector containerChildren(SymbolGroupNode *node, int type,
                                                   int size, const SymbolGroupValueContext &ctx)
{
    if (!size)
        return AbstractSymbolGroupNodePtrVector();
    if (size > 100)
        size = 100;
    switch (type) {
    case KT_QVector:
        return qVectorChildList(node, size, ctx);
    case KT_StdVector:
        return stdVectorChildList(node, size, ctx);
    case KT_QLinkedList:
        return qLinkedListChildList(node, size, ctx);
    case KT_QList:
        return qListChildList(SymbolGroupValue(node, ctx), size);
    case KT_QQueue:
        if (const SymbolGroupValue qList = SymbolGroupValue(node, ctx)[unsigned(0)])
            return qListChildList(qList, size);
        break;
    case KT_QStack:
        if (const SymbolGroupValue qVector = SymbolGroupValue(node, ctx)[unsigned(0)])
            return qVectorChildList(qVector.node(), size, ctx);
        break;
    case KT_QStringList:
        if (const SymbolGroupValue qList = SymbolGroupValue(node, ctx)[unsigned(0)])
            return qListChildList(qList, size);
        break;
    case KT_StdList:
        return stdListChildList(node, size , ctx);
    }
    return AbstractSymbolGroupNodePtrVector();
}
