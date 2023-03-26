// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "objectid.h"

#include <QMap>

namespace qark {
namespace impl {

class LoadingRefMap
{
public:
    bool hasObject(const ObjectId &id)
    {
        return m_references.find(id) != m_references.end();
    }

    template<typename T>
    T object(const ObjectId &id)
    {
        return reinterpret_cast<T>(m_references.value(id));
    }

    template<typename T>
    void addObject(const ObjectId &id, T *p)
    {
        m_references[id] = reinterpret_cast<void *>(p);
    }

private:
    typedef ObjectId KeyType;
    typedef void *ValueType;
    typedef QMap<KeyType, ValueType> MapType;

    MapType m_references;
};

} // namespace impl
} // namespace qark
