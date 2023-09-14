// Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ilocatorfilter.h"

class JavaScriptEngine;

namespace Core::Internal {

class JavaScriptFilter : public Core::ILocatorFilter
{
public:
    JavaScriptFilter();
    ~JavaScriptFilter();

private:
    LocatorMatcherTasks matchers() final;
    std::unique_ptr<JavaScriptEngine> m_javaScriptEngine;
};

} // namespace Core::Internal
