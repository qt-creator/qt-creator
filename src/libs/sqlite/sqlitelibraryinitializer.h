// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqliteglobal.h"

namespace Sqlite {

class LibraryInitializer
{
public:
    SQLITE_EXPORT static void initialize();

private:
    LibraryInitializer();
    ~LibraryInitializer() noexcept(false);
};

} // namespace Sqlite
