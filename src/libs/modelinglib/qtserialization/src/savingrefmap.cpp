// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qark/impl/savingrefmap.h"

namespace qark {
namespace impl {

int SavingRefMap::countDanglingReferences()
{
    int dangling = 0;
    for (auto it = m_references.begin(); it != m_references.end(); ++it) {
        if (!it.value().second)
            ++dangling;
    }
    return dangling;
}

bool SavingRefMap::hasRef(const void *address, const char *typeName)
{
    return m_references.constFind(KeyType(address, typeName)) != m_references.constEnd();
}

bool SavingRefMap::hasDefinedRef(const void *address, const char *typeName)
{
    const MapType::const_iterator it = m_references.constFind(KeyType(address, typeName));
    if (it == m_references.constEnd())
        return false;
    return it.value().second;
}

ObjectId SavingRefMap::ref(const void *address, const char *typeName, bool define)
{
    KeyType k = KeyType(address, typeName);
    MapType::iterator it = m_references.find(k);
    if (it != m_references.end()) {
        if (define)
            it.value().second = true;
        return it.value().first;
    }
    ObjectId id = m_nextRef++;
    m_references[k] = ValueType(id, define);
    return id;
}

} // namespace impl
} // namespace qark
