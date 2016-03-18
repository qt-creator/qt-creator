/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
