// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqliteexception.h"

namespace Sqlite {

class SQLITE_EXPORT SqlStatementBuilderException : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

} // namespace Sqlite
