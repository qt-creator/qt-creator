// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Todo::Internal {

class Settings;

class TodoOptionsPage final : public Core::IOptionsPage
{
public:
    TodoOptionsPage(Settings *settings, const std::function<void()> &onApply);
};

} // Todo::Internal
