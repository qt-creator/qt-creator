// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "objectid.h"

#include <QMap>
#include <QPair>
#include <typeinfo>

namespace qark {
namespace impl {

class SavingRefMap
{
public:
    template<typename T>
    bool hasRef(const T *object)
    {
        return hasRef(reinterpret_cast<const void *>(object), typeid(*object).name());
    }

    template<typename T>
    bool hasDefinedRef(const T *object)
    {
        return hasDefinedRef(reinterpret_cast<const void *>(object), typeid(*object).name());
    }

    template<typename T>
    ObjectId ref(const T *object, bool define = false)
    {
        return ref(reinterpret_cast<const void *>(object), typeid(*object).name(), define);
    }

    int countDanglingReferences();

private:
    bool hasRef(const void *address, const char *typeName);
    bool hasDefinedRef(const void *address, const char *typeName);
    ObjectId ref(const void *address, const char *typeName, bool define);

    typedef QPair<const void *, const char *> KeyType;
    typedef QPair<ObjectId, bool> ValueType;
    typedef QMap<KeyType, ValueType> MapType;

    MapType m_references;
    ObjectId m_nextRef = ObjectId(1);
};

} // namespace impl
} // namespace qark
