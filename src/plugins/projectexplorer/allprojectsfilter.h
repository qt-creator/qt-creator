// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/basefilefilter.h>

namespace ProjectExplorer {
namespace Internal {

// TODO: Don't derive from BaseFileFilter, flatten the hierarchy
class AllProjectsFilter : public Core::BaseFileFilter
{
    Q_OBJECT

public:
    AllProjectsFilter();
    void prepareSearch(const QString &entry) override;

private:
    Core::LocatorMatcherTasks matchers() final { return {m_cache.matcher()}; }
    // TODO: Remove me, replace with direct "m_cache.invalidate()" call
    void invalidateCache();
    Core::LocatorFileCache m_cache;
};

} // namespace Internal
} // namespace ProjectExplorer
