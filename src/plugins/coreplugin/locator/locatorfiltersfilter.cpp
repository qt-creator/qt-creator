/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "locator.h"
#include "locatorfiltersfilter.h"
#include "locatorwidget.h"

#include <coreplugin/coreconstants.h>

using namespace Core;
using namespace Core::Internal;

Q_DECLARE_METATYPE(ILocatorFilter*)

LocatorFiltersFilter::LocatorFiltersFilter(Locator *plugin,
                                               LocatorWidget *locatorWidget):
    m_plugin(plugin),
    m_locatorWidget(locatorWidget),
    m_icon(QIcon(QLatin1String(Core::Constants::ICON_NEXT)))
{
    setId("FiltersFilter");
    setDisplayName(tr("Available filters"));
    setIncludedByDefault(true);
    setHidden(true);
    setPriority(High);
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

QList<LocatorFilterEntry> LocatorFiltersFilter::matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    Q_UNUSED(entry) // search is already done in the GUI thread in prepareSearch
    QList<LocatorFilterEntry> entries;
    for (int i = 0; i < m_filterShortcutStrings.size(); ++i) {
        if (future.isCanceled())
            break;
        LocatorFilterEntry filterEntry(this,
                                m_filterShortcutStrings.at(i),
                                m_filterShortcutStrings.at(i),
                                m_icon);
        filterEntry.extraInfo = m_filterDisplayNames.at(i);
        entries.append(filterEntry);
    }
    return entries;
}

void LocatorFiltersFilter::accept(LocatorFilterEntry selection) const
{
    const QString shortcutString = selection.internalData.toString();
    if (!shortcutString.isEmpty())
        m_locatorWidget->show(shortcutString + QLatin1Char(' '),
                              shortcutString.length() + 1);
}

void LocatorFiltersFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    // Nothing to refresh
}
