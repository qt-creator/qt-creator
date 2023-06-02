// Copyright (C) 2016 Kläralvdalens Datakonsult AB, a KDAB Group company.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace CMakeProjectManager::Internal {

class CMakeBuildTargetFilter : Core::ILocatorFilter
{
public:
    CMakeBuildTargetFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

class CMakeOpenTargetFilter : Core::ILocatorFilter
{
public:
    CMakeOpenTargetFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

} // namespace CMakeProjectManager::Internal
