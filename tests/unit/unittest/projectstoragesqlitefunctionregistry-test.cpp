// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include <projectstorage/projectstoragesqlitefunctionregistry.h>

namespace {

class ProjectStorageSqliteFunctionRegistry : public testing::Test
{
protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    QmlDesigner::ProjectStorageSqliteFunctionRegistry registry{database};
};

TEST_F(ProjectStorageSqliteFunctionRegistry, ReturnsUnqualifiedType)
{
    std::lock_guard lock{database};
    Sqlite::ReadStatement<1> statement{"SELECT unqualifiedTypeName('Foo.Bar')", database};

    auto typeName = statement.value<Utils::SmallString>();

    ASSERT_THAT(typeName, Eq("Bar"));
}

TEST_F(ProjectStorageSqliteFunctionRegistry, ReturnsWholeStringIfNotDotIsFound)
{
    std::lock_guard lock{database};
    Sqlite::ReadStatement<1> statement{"SELECT unqualifiedTypeName('Foo_Bar')", database};

    auto typeName = statement.value<Utils::SmallString>();

    ASSERT_THAT(typeName, Eq("Foo_Bar"));
}

TEST_F(ProjectStorageSqliteFunctionRegistry, ReturnEmptyStringForEmptyInput)
{
    std::lock_guard lock{database};
    Sqlite::ReadStatement<1> statement{"SELECT unqualifiedTypeName('')", database};

    auto typeName = statement.value<Utils::SmallString>();

    ASSERT_THAT(typeName, IsEmpty());
}

TEST_F(ProjectStorageSqliteFunctionRegistry, ThrowsErrorForInteger)
{
    std::lock_guard lock{database};
    Sqlite::ReadStatement<1> statement("SELECT unqualifiedTypeName(1)", database);

    ASSERT_THROW(statement.value<Utils::SmallString>(), Sqlite::StatementHasError);
}

TEST_F(ProjectStorageSqliteFunctionRegistry, ThrowsErrorForFloat)
{
    std::lock_guard lock{database};
    Sqlite::ReadStatement<1> statement("SELECT unqualifiedTypeName(1.4)", database);

    ASSERT_THROW(statement.value<Utils::SmallString>(), Sqlite::StatementHasError);
}

TEST_F(ProjectStorageSqliteFunctionRegistry, ThrowsErrorForBlob)
{
    std::lock_guard lock{database};
    Sqlite::ReadStatement<1> statement("SELECT unqualifiedTypeName(x'0500')", database);

    ASSERT_THROW(statement.value<Utils::SmallString>(), Sqlite::StatementHasError);
}

TEST_F(ProjectStorageSqliteFunctionRegistry, ThrowsErrorForNull)
{
    std::lock_guard lock{database};
    Sqlite::ReadStatement<1> statement("SELECT unqualifiedTypeName(NULL)", database);

    ASSERT_THROW(statement.value<Utils::SmallString>(), Sqlite::StatementHasError);
}

TEST_F(ProjectStorageSqliteFunctionRegistry, ThrowsErrorForNoArgument)
{
    std::lock_guard lock{database};

    ASSERT_THROW(Sqlite::ReadStatement<1> statement("SELECT unqualifiedTypeName()", database),
                 Sqlite::StatementHasError);
}

TEST_F(ProjectStorageSqliteFunctionRegistry, ThrowsErrorForToManyArgument)
{
    std::lock_guard lock{database};

    ASSERT_THROW(Sqlite::ReadStatement<1> statement("SELECT unqualifiedTypeName('foo', 'bar')",
                                                    database),
                 Sqlite::StatementHasError);
}
} // namespace
