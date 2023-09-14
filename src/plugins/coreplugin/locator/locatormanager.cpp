// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "locatormanager.h"

#include "ilocatorfilter.h"
#include "locator.h"
#include "locatorwidget.h"
#include "../icore.h"

#include <aggregation/aggregate.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QApplication>

using namespace Core::Internal;

namespace Core {

/*!
    \class Core::LocatorManager
    \inmodule QtCreator
    \internal
*/

LocatorManager::LocatorManager()
{
}

static LocatorWidget *locatorWidget()
{
    static QPointer<LocatorPopup> popup;
    QWidget *window = ICore::dialogParent()->window();
    // if that is a popup, try to find a better one
    if (window->windowFlags() & Qt::Popup && window->parentWidget())
        window = window->parentWidget()->window();
    if (!Locator::useCenteredPopupForShortcut()) {
        if (auto *widget = Aggregation::query<LocatorWidget>(window)) {
            if (popup)
                popup->close();
            return widget;
        }
    }
    if (!popup) {
        popup = createLocatorPopup(Locator::instance(), window);
        popup->show();
    }
    return popup->inputWidget();
}

void LocatorManager::showFilter(ILocatorFilter *filter)
{
    Locator::showFilter(filter, locatorWidget());
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

bool LocatorManager::locatorHasFocus()
{
    QWidget *w = qApp->focusWidget();
    while (w) {
        if (qobject_cast<LocatorWidget *>(w))
            return true;
        w = w->parentWidget();
    }
    return false;
}

} // namespace Core
