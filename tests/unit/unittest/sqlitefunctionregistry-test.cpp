// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include <sqlitefunctionregistry.h>

namespace {

class SqliteFunctionRegistry : public testing::Test
{
public:
    SqliteFunctionRegistry() { Sqlite::FunctionRegistry::registerPathExists(database); }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
};

TEST_F(SqliteFunctionRegistry, PathExists)
{
    std::lock_guard lock{database};
    Sqlite::ReadStatement<1> statement{"SELECT pathExists('" TESTDATA_DIR "/sqlite_database.db')",
                                       database};

    auto pathExists = statement.value<bool>();

    ASSERT_TRUE(pathExists);
}

TEST_F(SqliteFunctionRegistry, PathDoesntExists)
{
    std::lock_guard lock{database};
    Sqlite::ReadStatement<1> statement{"SELECT pathExists('" TESTDATA_DIR "/sqlite_database2.db')",
                                       database};

    auto pathExists = statement.value<bool>();

    ASSERT_FALSE(pathExists);
}

} // namespace
