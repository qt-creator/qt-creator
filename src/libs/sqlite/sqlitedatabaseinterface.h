// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/smallstringvector.h>

#include "sqliteglobal.h"

namespace Sqlite {
class DatabaseInterface
{
public:
    virtual void walCheckpointFull() = 0;
    virtual void execute(Utils::SmallStringView sqlStatement) = 0;
    virtual void setUpdateHook(
        void *object,
        void (*)(void *object, int, char const *database, char const *, long long rowId))
        = 0;
    virtual void resetUpdateHook() = 0;
    virtual void applyAndUpdateSessions() = 0;
    virtual void setAttachedTables(const Utils::SmallStringVector &tables) = 0;

protected:
    ~DatabaseInterface() = default;
};
} // namespace Sqlite
