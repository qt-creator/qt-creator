// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
