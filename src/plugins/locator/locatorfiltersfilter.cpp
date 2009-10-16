/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "locatorfiltersfilter.h"
#include "locatorplugin.h"
#include "locatorwidget.h"

#include <coreplugin/coreconstants.h>

using namespace Locator;
using namespace Locator::Internal;

Q_DECLARE_METATYPE(ILocatorFilter*);

LocatorFiltersFilter::LocatorFiltersFilter(LocatorPlugin *plugin,
                                               LocatorWidget *locatorWidget):
    m_plugin(plugin),
    m_locatorWidget(locatorWidget),
    m_icon(QIcon(Core::Constants::ICON_NEXT))
{
    setIncludedByDefault(true);
    setHidden(true);
}

QString LocatorFiltersFilter::trName() const
{
    return tr("Available filters");
}

QString LocatorFiltersFilter::name() const
{
    return QLatin1String("FiltersFilter");
}

ILocatorFilter::Priority LocatorFiltersFilter::priority() const
{
    return High;
}

QList<FilterEntry> LocatorFiltersFilter::matchesFor(const QString &entry)
{
    QList<FilterEntry> entries;
    if (entry.isEmpty()) {
        foreach (ILocatorFilter *filter, m_plugin->filters()) {
            if (!filter->shortcutString().isEmpty() && !filter->isHidden()) {
                FilterEntry entry(this,
                                  filter->shortcutString(),
                                  QVariant::fromValue(filter),
                                  m_icon);
                entry.extraInfo = filter->trName();
                entries.append(entry);
            }
        }
    }
    return entries;
}

void LocatorFiltersFilter::accept(FilterEntry selection) const
{
    ILocatorFilter *filter = selection.internalData.value<ILocatorFilter*>();
    if (filter)
        m_locatorWidget->show(filter->shortcutString() + " ",
                           filter->shortcutString().length() + 1);
}

void LocatorFiltersFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    // Nothing to refresh
}

bool LocatorFiltersFilter::isConfigurable() const
{
    return false;
}
