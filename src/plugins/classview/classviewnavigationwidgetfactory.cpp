// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "classviewnavigationwidgetfactory.h"

#include "classviewnavigationwidget.h"

#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>

#include <QSettings>

namespace ClassView {
namespace Internal {

///////////////////////////////// NavigationWidgetFactory //////////////////////////////////

/*!
    \class NavigationWidgetFactory
    \brief The NavigationWidgetFactory class implements a singleton instance of
    the INavigationWidgetFactory for the Class View.

    Supports the \c setState public slot for adding the widget factory to or
    removing it from \c ExtensionSystem::PluginManager.
*/

NavigationWidgetFactory::NavigationWidgetFactory()
{
    setDisplayName(tr("Class View"));
    setPriority(500);
    setId("Class View");
}

Core::NavigationView NavigationWidgetFactory::createWidget()
{
    auto widget = new NavigationWidget();
    return {widget, widget->createToolButtons()};
}


/*!
   Returns a settings prefix for \a position.
*/
static QString settingsPrefix(int position)
{
    return QString::fromLatin1("ClassView.Treewidget.%1.FlatMode").arg(position);
}

//! Flat mode settings

void NavigationWidgetFactory::saveSettings(Utils::QtcSettings *settings,
                                           int position,
                                           QWidget *widget)
{
    auto pw = qobject_cast<NavigationWidget *>(widget);
    QTC_ASSERT(pw, return);

    // .beginGroup is not used - to prevent simultaneous access
    QString settingsGroup = settingsPrefix(position);
    settings->setValue(settingsGroup, pw->flatMode());
}

void NavigationWidgetFactory::restoreSettings(QSettings *settings, int position, QWidget *widget)
{
    auto pw = qobject_cast<NavigationWidget *>(widget);
    QTC_ASSERT(pw, return);

    // .beginGroup is not used - to prevent simultaneous access
    QString settingsGroup = settingsPrefix(position);
    pw->setFlatMode(settings->value(settingsGroup, false).toBool());
}

} // namespace Internal
} // namespace ClassView
