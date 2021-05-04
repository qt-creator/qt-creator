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
