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

#include "projectstoragesqlitefunctionregistry.h"

#include "sqlite.h"

namespace QmlDesigner {

extern "C" {
namespace {
void unqualifiedTypeName(sqlite3_context *context, int, sqlite3_value **arguments)
{
    auto argument = arguments[0];

    auto errorText = "unqualifiedTypeName only accepts text";

    if (sqlite3_value_type(argument) != SQLITE_TEXT) {
        sqlite3_result_error(context, errorText, int(std::char_traits<char>::length(errorText)));
        return;
    }

    auto size = sqlite3_value_bytes(argument);

    auto content = reinterpret_cast<const char *>(sqlite3_value_text(argument));

    auto begin = content;
    auto end = content + size;

    auto rbegin = std::make_reverse_iterator(end);
    auto rend = std::make_reverse_iterator(begin);

    auto found = std::find(rbegin, rend, '.').base();

    auto unqualifiedSize = end - found;

    sqlite3_result_text(context, found, int(unqualifiedSize), SQLITE_STATIC);
}

void registerUnqualifiedTypeName(sqlite3 *database)
{
    sqlite3_create_function(database,
                            "unqualifiedTypeName",
                            1,
                            SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                            nullptr,
                            unqualifiedTypeName,
                            nullptr,
                            nullptr);
}
} // namespace
}

ProjectStorageSqliteFunctionRegistry::ProjectStorageSqliteFunctionRegistry(Sqlite::Database &database)
{
    auto databaseHandle = database.backend().sqliteDatabaseHandle();

    registerUnqualifiedTypeName(databaseHandle);
}

} // namespace QmlDesigner
