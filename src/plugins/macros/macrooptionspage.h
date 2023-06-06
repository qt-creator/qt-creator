// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Macros::Internal {

class MacroOptionsPage final : public Core::IOptionsPage
{
public:
    MacroOptionsPage();
};

} // Macros::Internal
