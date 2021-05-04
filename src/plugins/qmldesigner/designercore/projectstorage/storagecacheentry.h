/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

namespace QmlDesigner {

template<typename Type, typename ViewType, typename IndexType>
class StorageCacheEntry
{
public:
    StorageCacheEntry(ViewType value, IndexType id)
        : value(value)
        , id(id)
    {}

    StorageCacheEntry(ViewType value, typename IndexType::DatabaseType id)
        : value(value)
        , id{id}
    {}

    operator ViewType() const noexcept { return value; }
    friend bool operator==(const StorageCacheEntry &first, const StorageCacheEntry &second)
    {
        return first.id == second.id && first.value == second.value;
    }

public:
    Type value;
    IndexType id;
};

} // namespace QmlDesigner
