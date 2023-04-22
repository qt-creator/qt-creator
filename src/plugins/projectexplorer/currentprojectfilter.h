// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/basefilefilter.h>

namespace ProjectExplorer {

class Project;

namespace Internal {

// TODO: Don't derive from BaseFileFilter, flatten the hierarchy
class CurrentProjectFilter : public Core::BaseFileFilter
{
    Q_OBJECT

public:
    CurrentProjectFilter();
    void prepareSearch(const QString &entry) override;

private:
    Core::LocatorMatcherTasks matchers() final { return {m_cache.matcher()}; }
    void currentProjectChanged();
    // TODO: Remove me, replace with direct "m_cache.invalidate()" call
    void invalidateCache();

    Core::LocatorFileCache m_cache;
    Project *m_project = nullptr;
};

} // namespace Internal
} // namespace ProjectExplorer
