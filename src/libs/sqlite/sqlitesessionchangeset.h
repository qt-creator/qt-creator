/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "sqliteglobal.h"

#include <utils/span.h>

#include <memory>
#include <vector>

#include <iosfwd>

namespace Sqlite {

class Sessions;

class SessionChangeSet
{
public:
    SessionChangeSet(Utils::span<const byte> blob);
    SessionChangeSet(Sessions &session);
    ~SessionChangeSet();
    SessionChangeSet(const SessionChangeSet &) = delete;
    void operator=(const SessionChangeSet &) = delete;
    SessionChangeSet(SessionChangeSet &&other) noexcept
    {
        SessionChangeSet temp;
        swap(temp, other);
        swap(temp, *this);
    }
    void operator=(SessionChangeSet &);

    Utils::span<const byte> asSpan() const;

    friend void swap(SessionChangeSet &first, SessionChangeSet &second) noexcept
    {
        SessionChangeSet temp;
        std::swap(temp.data, first.data);
        std::swap(temp.size, first.size);
        std::swap(first.data, second.data);
        std::swap(first.size, second.size);
        std::swap(temp.data, second.data);
        std::swap(temp.size, second.size);
    }

private:
    SessionChangeSet() = default;

public:
    void *data = nullptr;
    int size = {};
};

using SessionChangeSets = std::vector<SessionChangeSet>;

} // namespace Sqlite
