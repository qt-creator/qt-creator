// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "locatorfiltersfilter.h"
#include "../actionmanager/actionmanager.h"

#include "locator.h"
#include "locatorwidget.h"

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

using namespace Core;
using namespace Core::Internal;

Q_DECLARE_METATYPE(ILocatorFilter*)

LocatorFiltersFilter::LocatorFiltersFilter():
    m_icon(Utils::Icons::NEXT.icon())
{
    setId("FiltersFilter");
    setDisplayName(tr("Available filters"));
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
        LocatorFilterEntry filterEntry(this,
                                m_filterShortcutStrings.at(i),
                                i,
                                m_icon);
        filterEntry.extraInfo = m_filterDisplayNames.at(i);
        filterEntry.toolTip = m_filterDescriptions.at(i);
        filterEntry.displayExtra = m_filterKeyboardShortcuts.at(i);
        entries.append(filterEntry);
    }
    return entries;
}

void LocatorFiltersFilter::accept(const LocatorFilterEntry &selection,
                                  QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(selectionLength)
    bool ok;
    int index = selection.internalData.toInt(&ok);
    QTC_ASSERT(ok && index >= 0 && index < m_filterShortcutStrings.size(), return);
    const QString shortcutString = m_filterShortcutStrings.at(index);
    if (!shortcutString.isEmpty()) {
        *newText = shortcutString + ' ';
        *selectionStart = shortcutString.length() + 1;
    }
}
