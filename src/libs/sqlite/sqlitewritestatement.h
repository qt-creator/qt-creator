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
    WriteStatement(Utils::SmallStringView sqlStatement, Database &database)
        : Base(sqlStatement, database)
    {
        checkIsWritableStatement();
        Base::checkBindingParameterCount(BindParameterCount);
        Base::checkColumnCount(0);
    }

    using Base::database;
    using Base::execute;
    using Base::write;

protected:
    void checkIsWritableStatement()
    {
        if (Base::isReadOnlyStatement())
            throw NotWriteSqlStatement();
    }
};

} // namespace Sqlite
