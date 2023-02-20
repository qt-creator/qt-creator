// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "sqlitefunctionregistry.h"

#include "sqlite.h"

#include <QFileInfo>

namespace {
extern "C" {
void pathExists(sqlite3_context *context, int, sqlite3_value **arguments)
{
    auto argument = arguments[0];

    auto errorText = "pathExists only accepts text";

    if (sqlite3_value_type(argument) != SQLITE_TEXT) {
        sqlite3_result_error(context, errorText, int(std::char_traits<char>::length(errorText)));
        return;
    }

    auto size = sqlite3_value_bytes(argument);

    auto content = QByteArrayView{sqlite3_value_text(argument), size};

    QString path = QString::fromUtf8(content);

    bool exists = QFileInfo::exists(path);

    sqlite3_result_int(context, exists);
}
}
} // namespace

namespace Sqlite::FunctionRegistry {

void registerPathExists(Sqlite::Database &database)
{
    sqlite3_create_function(database.backend().sqliteDatabaseHandle(),
                            "pathExists",
                            1,
                            SQLITE_UTF8 | SQLITE_INNOCUOUS,
                            nullptr,
                            pathExists,
                            nullptr,
                            nullptr);
}

} // namespace Sqlite::FunctionRegistry
