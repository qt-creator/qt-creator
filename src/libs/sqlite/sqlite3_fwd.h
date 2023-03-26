// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#if defined(SQLITE_STATIC_LIBRARY) || defined(SQLITEC_STATIC_LIBRARY)
using sqlite3 = struct qtc_sqlite3;
using sqlite3_stmt = struct qtc_sqlite3_stmt;
using sqlite3_session = struct qtc_sqlite3_session;
using sqlite3_changeset_iter = struct qtc_sqlite3_changeset_iter;
#else
using sqlite3 = struct sqlite3;
using sqlite3_stmt = struct sqlite3_stmt;
using sqlite3_session = struct sqlite3_session;
using sqlite3_changeset_iter = struct sqlite3_changeset_iter;
#endif
