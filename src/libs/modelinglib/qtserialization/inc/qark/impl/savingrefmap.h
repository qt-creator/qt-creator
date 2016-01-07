/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QARK_SAVINGREFMAP_H
#define QARK_SAVINGREFMAP_H

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

#endif // QARK_SAVINGREFMAP_H
