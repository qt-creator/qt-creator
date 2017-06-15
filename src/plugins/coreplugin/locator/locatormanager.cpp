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

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

namespace Core {

static Internal::LocatorWidget *m_locatorWidget = 0;

LocatorManager::LocatorManager(Internal::LocatorWidget *locatorWidget)
  : QObject(locatorWidget)
{
    m_locatorWidget = locatorWidget;
}

void LocatorManager::showFilter(ILocatorFilter *filter)
{
    QTC_ASSERT(filter, return);
    QTC_ASSERT(m_locatorWidget, return);
    QString searchText = tr("<type here>");
    const QString currentText = m_locatorWidget->currentText().trimmed();
    // add shortcut string at front or replace existing shortcut string
    if (!currentText.isEmpty()) {
        searchText = currentText;
        foreach (ILocatorFilter *otherfilter, Internal::Locator::filters()) {
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
    QTC_ASSERT(m_locatorWidget, return);
    m_locatorWidget->showText(text, selectionStart, selectionLength);
}

} // namespace Internal
