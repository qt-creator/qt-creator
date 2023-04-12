// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "locatorfiltersfilter.h"

#include "locator.h"
#include "../actionmanager/actionmanager.h"
#include "../coreplugintr.h"

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

Q_DECLARE_METATYPE(Core::ILocatorFilter*)

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

    const auto onSetup = [=] {
        if (!storage->input().isEmpty())
            return true;

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
                entry.displayIcon = m_icon;
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
        return true;
    };
    return {{Sync(onSetup), storage}};
}

void LocatorFiltersFilter::prepareSearch(const QString &entry)
{
    m_filterShortcutStrings.clear();
    m_filterDisplayNames.clear();
    m_filterDescriptions.clear();
    if (!entry.isEmpty())
        return;

    QMap<QString, ILocatorFilter *> uniqueFilters;
    const QList<ILocatorFilter *> allFilters = Locator::filters();
    for (ILocatorFilter *filter : allFilters) {
        const QString filterId = filter->shortcutString() + ',' + filter->displayName();
        uniqueFilters.insert(filterId, filter);
    }

    for (ILocatorFilter *filter : std::as_const(uniqueFilters)) {
        if (!filter->shortcutString().isEmpty() && !filter->isHidden() && filter->isEnabled()) {
            m_filterShortcutStrings.append(filter->shortcutString());
            m_filterDisplayNames.append(filter->displayName());
            m_filterDescriptions.append(filter->description());
            QString keyboardShortcut;
            if (auto command = ActionManager::command(filter->actionId()))
                keyboardShortcut = command->keySequence().toString(QKeySequence::NativeText);
            m_filterKeyboardShortcuts.append(keyboardShortcut);
        }
    }
}

QList<LocatorFilterEntry> LocatorFiltersFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &entry)
{
    Q_UNUSED(entry) // search is already done in the GUI thread in prepareSearch
    QList<LocatorFilterEntry> entries;
    for (int i = 0; i < m_filterShortcutStrings.size(); ++i) {
        if (future.isCanceled())
            break;
        const QString shortcutString = m_filterShortcutStrings.at(i);
        LocatorFilterEntry filterEntry;
        filterEntry.displayName = shortcutString;
        filterEntry.acceptor = [shortcutString] {
            return AcceptResult{shortcutString + ' ', int(shortcutString.size() + 1)};
        };
        filterEntry.displayIcon = m_icon;
        filterEntry.extraInfo = m_filterDisplayNames.at(i);
        filterEntry.toolTip = m_filterDescriptions.at(i);
        filterEntry.displayExtra = m_filterKeyboardShortcuts.at(i);
        entries.append(filterEntry);
    }
    return entries;
}

} // Core::Internal
