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

#include "sqlitesessionchangeset.h"
#include "sqlitesessions.h"

#include <utils/smallstringio.h>

#include <sqlite3ext.h>

namespace Sqlite {

namespace {
void checkResultCode(int resultCode)
{
    switch (resultCode) {
    case SQLITE_NOMEM:
        throw std::bad_alloc();
    }

    if (resultCode != SQLITE_OK)
        throw UnknowError("Unknow exception");
}

} // namespace

SessionChangeSet::SessionChangeSet(Utils::span<const byte> blob)
    : data(sqlite3_malloc64(blob.size()))
    , size(int(blob.size()))
{
    std::memcpy(data, blob.data(), blob.size());
}

SessionChangeSet::SessionChangeSet(Sessions &session)
{
    int resultCode = sqlite3session_changeset(session.session.get(), &size, &data);
    checkResultCode(resultCode);
}

SessionChangeSet::~SessionChangeSet()
{
    sqlite3_free(data);
}

Utils::span<const byte> SessionChangeSet::asSpan() const
{
    return {static_cast<const byte *>(data), static_cast<std::size_t>(size)};
}

} // namespace Sqlite
