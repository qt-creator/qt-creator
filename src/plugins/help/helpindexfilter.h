// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace Help::Internal {

class HelpIndexFilter final : public Core::ILocatorFilter
{
public:
    HelpIndexFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
    void invalidateCache();

    QStringList m_allIndicesCache;
    QStringList m_lastIndicesCache;
    QString m_lastEntry;
    bool m_needsUpdate = true;
    QIcon m_icon;
};

} // namespace Help::Internal
