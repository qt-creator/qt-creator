// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace Macros::Internal {

class MacroLocatorFilter : public Core::ILocatorFilter
{
public:
    MacroLocatorFilter();

private:
    Core::LocatorMatcherTasks matchers() final;

    const QIcon m_icon;
};

} // namespace Macros::Internal
