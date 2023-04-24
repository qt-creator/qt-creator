// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ilocatorfilter.h"

namespace Core::Internal {

/*!
  This filter provides the user with the list of available Locator filters.
  The list is only shown when nothing has been typed yet.
 */
class LocatorFiltersFilter : public ILocatorFilter
{
public:
    LocatorFiltersFilter();

private:
    LocatorMatcherTasks matchers() final;

    QStringList m_filterShortcutStrings;
    QStringList m_filterDisplayNames;
    QStringList m_filterDescriptions;
    QStringList m_filterKeyboardShortcuts;
    QIcon m_icon;
};

} // namespace Core::Internal
