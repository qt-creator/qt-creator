// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace Autotest::Internal {

class DataTagLocatorFilter final : public Core::ILocatorFilter
{
public:
    DataTagLocatorFilter();

    Core::LocatorMatcherTasks matchers() final;
};

} // namespace Autotest::Internal
