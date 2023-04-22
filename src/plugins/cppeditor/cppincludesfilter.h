// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/basefilefilter.h>

namespace CppEditor::Internal {

class CppIncludesFilter : public Core::BaseFileFilter
{
public:
    CppIncludesFilter();

public:
    void prepareSearch(const QString &entry) override;

private:
    Core::LocatorMatcherTasks matchers() final { return {m_cache.matcher()}; }
    void invalidateCache();

    Core::LocatorFileCache m_cache;
    bool m_needsUpdate = true;
};

} // namespace CppEditor::Internal
