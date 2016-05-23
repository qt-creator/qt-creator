/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "locator.h"
#include "locatorfiltersfilter.h"
#include "locatorwidget.h"

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

using namespace Core;
using namespace Core::Internal;

Q_DECLARE_METATYPE(ILocatorFilter*)

LocatorFiltersFilter::LocatorFiltersFilter(Locator *plugin,
                                               LocatorWidget *locatorWidget):
    m_plugin(plugin),
    m_locatorWidget(locatorWidget),
    m_icon(Utils::Icons::NEXT.icon())
{
    setId("FiltersFilter");
    setDisplayName(tr("Available filters"));
    setIncludedByDefault(true);
    setHidden(true);
    setPriority(Highest);
    setConfigurable(false);
}

void LocatorFiltersFilter::prepareSearch(const QString &entry)
{
    m_filterShortcutStrings.clear();
    m_filterDisplayNames.clear();
    if (!entry.isEmpty())
        return;

    QMap<QString, ILocatorFilter *> uniqueFilters;
    foreach (ILocatorFilter *filter, m_plugin->filters()) {
        const QString filterId = filter->shortcutString() + QLatin1Char(',') + filter->displayName();
        uniqueFilters.insert(filterId, filter);
    }

    foreach (ILocatorFilter *filter, uniqueFilters) {
        if (!filter->shortcutString().isEmpty() && !filter->isHidden() && filter->isEnabled()) {
            m_filterShortcutStrings.append(filter->shortcutString());
            m_filterDisplayNames.append(filter->displayName());
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
        entries.append(filterEntry);
    }
    return entries;
}

void LocatorFiltersFilter::accept(LocatorFilterEntry selection) const
{
    bool ok;
    int index = selection.internalData.toInt(&ok);
    QTC_ASSERT(ok && index >= 0 && index < m_filterShortcutStrings.size(), return);
    const QString shortcutString = m_filterShortcutStrings.at(index);
    if (!shortcutString.isEmpty())
        m_locatorWidget->show(shortcutString + QLatin1Char(' '),
                              shortcutString.length() + 1);
}

void LocatorFiltersFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    // Nothing to refresh
}
