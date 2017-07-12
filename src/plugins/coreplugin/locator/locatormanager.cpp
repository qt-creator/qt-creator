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

#include "locatormanager.h"

#include "ilocatorfilter.h"
#include "locator.h"
#include "locatorwidget.h"

#include <aggregation/aggregate.h>
#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

using namespace Core::Internal;

namespace Core {

LocatorManager::LocatorManager(QObject *parent)
  : QObject(parent)
{
}

static LocatorWidget *locatorWidget()
{
    static QPointer<LocatorPopup> popup;
    QWidget *window = ICore::dialogParent()->window();
    if (auto *widget = Aggregation::query<LocatorWidget>(window)) {
        if (popup)
            popup->close();
        return widget;
    }
    if (!popup) {
        popup = createLocatorPopup(Locator::instance(), window);
        popup->show();
    }
    return popup->inputWidget();
}

void LocatorManager::showFilter(ILocatorFilter *filter)
{
    QTC_ASSERT(filter, return);
    QString searchText = tr("<type here>");
    const QString currentText = locatorWidget()->currentText().trimmed();
    // add shortcut string at front or replace existing shortcut string
    if (!currentText.isEmpty()) {
        searchText = currentText;
        foreach (ILocatorFilter *otherfilter, Locator::filters()) {
            if (currentText.startsWith(otherfilter->shortcutString() + QLatin1Char(' '))) {
                searchText = currentText.mid(otherfilter->shortcutString().length() + 1);
                break;
            }
        }
    }
    show(filter->shortcutString() + QLatin1Char(' ') + searchText,
         filter->shortcutString().length() + 1,
         searchText.length());
}

void LocatorManager::show(const QString &text,
                          int selectionStart, int selectionLength)
{
    locatorWidget()->showText(text, selectionStart, selectionLength);
}

QWidget *LocatorManager::createLocatorInputWidget(QWidget *window)
{
    auto locatorWidget = createStaticLocatorWidget(Locator::instance());
    // register locator widget for this window
    auto agg = new Aggregation::Aggregate;
    agg->add(window);
    agg->add(locatorWidget);
    return locatorWidget;
}

} // namespace Core
