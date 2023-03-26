// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "sqlitedatabase.h"

namespace Sqlite::FunctionRegistry {

SQLITE_EXPORT void registerPathExists(Sqlite::Database &database);

} // namespace Sqlite::FunctionRegistry
