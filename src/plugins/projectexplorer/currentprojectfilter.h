// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace ProjectExplorer { class Project; }

namespace ProjectExplorer::Internal {

class CurrentProjectFilter : public Core::ILocatorFilter
{
public:
    CurrentProjectFilter();

private:
    Core::LocatorMatcherTasks matchers() final { return {m_cache.matcher()}; }
    void currentProjectChanged();
    void invalidate() { m_cache.invalidate(); }

    Core::LocatorFileCache m_cache;
    Project *m_project = nullptr;
};

} // namespace ProjectExplorer::Internal
