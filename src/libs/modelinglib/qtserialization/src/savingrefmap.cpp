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
