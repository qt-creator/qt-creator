// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqliteexception.h"

namespace Sqlite {

class SQLITE_EXPORT SqlStatementBuilderException : public ExceptionWithMessage
{
public:
    SqlStatementBuilderException(const char *whatHasHappened,
                                 Utils::SmallString &&sqliteErrorMessage)
        : ExceptionWithMessage(std::move(sqliteErrorMessage))
        , m_whatHasHappened{whatHasHappened}
    {}

    const char *what() const noexcept override;

private:
    const char *m_whatHasHappened;
};

} // namespace Sqlite
