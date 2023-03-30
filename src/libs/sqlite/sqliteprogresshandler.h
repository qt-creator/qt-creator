// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "sqlitedatabase.h"

namespace Sqlite {

class ProgressHandler
{
public:
    template<typename Callable>
    ProgressHandler(Callable &&callable, int operationCount, Database &database)
        : m_database{database}
    {
        std::unique_lock<TransactionInterface> locker{m_database};
        m_database.backend().setProgressHandler(operationCount, std::forward<Callable>(callable));
    }

    ~ProgressHandler()
    {
        std::unique_lock<TransactionInterface> locker{m_database};
        m_database.backend().resetProgressHandler();
    }

private:
    Database &m_database;
};

} // namespace Sqlite
