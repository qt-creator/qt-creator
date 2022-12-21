// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/smallstringfwd.h>

#include <QtGlobal>

#if defined(SQLITE_LIBRARY)
#  define SQLITE_EXPORT Q_DECL_EXPORT
#elif defined(SQLITE_STATIC_LIBRARY)
#  define SQLITE_EXPORT
#else
#  define SQLITE_EXPORT Q_DECL_IMPORT
#endif

namespace Sqlite {

enum class ColumnType : char { None, Numeric, Integer, Real, Text, Blob };
enum class StrictColumnType : char { Any, Integer, Int, Real, Text, Blob };

enum class ConstraintType : char { NoConstraint, PrimaryKey, Unique, ForeignKey };

enum class ForeignKeyAction : char { NoAction, Restrict, SetNull, SetDefault, Cascade };

enum class Enforment : char { Immediate, Deferred };

enum class ColumnConstraint : char { PrimaryKey };

enum class JournalMode : char
{
    Delete,
    Truncate,
    Persist,
    Memory,
    Wal
};

enum class LockingMode : char { Default, Normal, Exclusive };

enum class OpenMode : char
{
    ReadOnly,
    ReadWrite
};

enum class ChangeType : int { Delete = 9, Insert = 18, Update = 23 };

enum class CallbackControl : unsigned char { Continue, Abort };

} // namespace Sqlite
