/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "locatorfiltersfilter.h"
#include "locatorplugin.h"
#include "locatorwidget.h"

#include <coreplugin/coreconstants.h>

using namespace Locator;
using namespace Locator::Internal;

Q_DECLARE_METATYPE(ILocatorFilter*)

LocatorFiltersFilter::LocatorFiltersFilter(LocatorPlugin *plugin,
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

QList<FilterEntry> LocatorFiltersFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry)
{
    QList<FilterEntry> entries;
    if (!entry.isEmpty())
        return entries;

    QMap<QString, ILocatorFilter*> uniqueFilters;
    foreach (ILocatorFilter *filter, m_plugin->filters()) {
        const QString filterId = filter->shortcutString() + QLatin1Char(',') + filter->displayName();
        uniqueFilters.insert(filterId, filter);
    }

    foreach (ILocatorFilter *filter, uniqueFilters) {
        if (future.isCanceled())
            break;
        if (!filter->shortcutString().isEmpty() && !filter->isHidden() && filter->isEnabled()) {
            FilterEntry filterEntry(this,
                                    filter->shortcutString(),
                                    QVariant::fromValue(filter),
                                    m_icon);
            filterEntry.extraInfo = filter->displayName();
            entries.append(filterEntry);
        }
    }

    return entries;
}

void LocatorFiltersFilter::accept(FilterEntry selection) const
{
    ILocatorFilter *filter = selection.internalData.value<ILocatorFilter*>();
    if (filter)
        m_locatorWidget->show(filter->shortcutString() + QLatin1Char(' '),
                           filter->shortcutString().length() + 1);
}

void LocatorFiltersFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    // Nothing to refresh
}
