// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Beautifier::Internal {

class ClangFormatSettings;

class ClangFormatOptionsPage final : public Core::IOptionsPage
{
public:
    explicit ClangFormatOptionsPage(ClangFormatSettings *settings);
};

} // Beautifier::Internal
