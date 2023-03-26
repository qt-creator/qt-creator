// Copyright (C) 2016 Lorenz Haas
// Copyright (C) 2022 Xavier BESSON
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace CMakeProjectManager::Internal {

class CMakeFormatterOptionsPage final : public Core::IOptionsPage
{
public:
    explicit CMakeFormatterOptionsPage();
};

} // CMakeProjectManager::Internal
