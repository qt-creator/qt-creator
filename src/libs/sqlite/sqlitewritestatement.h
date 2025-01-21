// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqlitebasestatement.h"

namespace Sqlite {
template<int BindParameterCount = 0>
class WriteStatement : protected StatementImplementation<BaseStatement, -1, BindParameterCount>
{
    using Base = StatementImplementation<BaseStatement, -1, BindParameterCount>;

public:
    WriteStatement(Utils::SmallStringView sqlStatement,
                   Database &database,
                   const source_location &sourceLocation = source_location::current())
        : Base(sqlStatement, database, sourceLocation)
    {
        checkIsWritableStatement(sourceLocation);
        Base::checkBindingParameterCount(BindParameterCount, sourceLocation);
        Base::checkColumnCount(0, sourceLocation);
    }

    using Base::database;
    using Base::execute;
    using Base::write;

protected:
    void checkIsWritableStatement(const source_location &sourceLocation)
    {
        if (Base::isReadOnlyStatement())
            throw NotWriteSqlStatement(sourceLocation);
    }
};

} // namespace Sqlite
