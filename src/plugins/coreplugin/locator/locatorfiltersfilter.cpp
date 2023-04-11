// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "locatorfiltersfilter.h"

#include "locator.h"
#include "../actionmanager/actionmanager.h"
#include "../coreplugintr.h"

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

Q_DECLARE_METATYPE(Core::ILocatorFilter*)

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
