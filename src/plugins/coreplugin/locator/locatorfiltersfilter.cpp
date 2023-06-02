// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "locatorfiltersfilter.h"

#include "locator.h"
#include "../actionmanager/actionmanager.h"
#include "../coreplugintr.h"

#include <utils/utilsicons.h>

using namespace Utils;

namespace Core::Internal {

LocatorFiltersFilter::LocatorFiltersFilter():
    m_icon(Utils::Icons::NEXT.icon())
{
    setId("FiltersFilter");
    setDisplayName(Tr::tr("Available filters"));
    setDefaultIncludedByDefault(true);
    setHidden(true);
    setPriority(Highest);
    setConfigurable(false);
}

LocatorMatcherTasks LocatorFiltersFilter::matchers()
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;

    const auto onSetup = [storage, icon = m_icon] {
        if (!storage->input().isEmpty())
            return;

        QMap<QString, ILocatorFilter *> uniqueFilters;
        const QList<ILocatorFilter *> allFilters = Locator::filters();
        for (ILocatorFilter *filter : allFilters) {
            const QString filterId = filter->shortcutString() + ',' + filter->displayName();
            uniqueFilters.insert(filterId, filter);
        }

        LocatorFilterEntries entries;
        for (ILocatorFilter *filter : std::as_const(uniqueFilters)) {
            const QString shortcutString = filter->shortcutString();
            if (!shortcutString.isEmpty() && !filter->isHidden() && filter->isEnabled()) {
                LocatorFilterEntry entry;
                entry.displayName = shortcutString;
                entry.acceptor = [shortcutString] {
                    return AcceptResult{shortcutString + ' ', int(shortcutString.size() + 1)};
                };
                entry.displayIcon = icon;
                entry.extraInfo = filter->displayName();
                entry.toolTip = filter->description();
                QString keyboardShortcut;
                if (auto command = ActionManager::command(filter->actionId()))
                    keyboardShortcut = command->keySequence().toString(QKeySequence::NativeText);
                entry.displayExtra = keyboardShortcut;
                entries.append(entry);
            }
        }
        storage->reportOutput(entries);
    };
    return {{Sync(onSetup), storage}};
}

} // Core::Internal
