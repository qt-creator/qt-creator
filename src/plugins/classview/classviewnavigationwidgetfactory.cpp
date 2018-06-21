/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov
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

#include "classviewnavigationwidgetfactory.h"
#include "classviewnavigationwidget.h"
#include "classviewconstants.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QKeySequence>
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
    Core::NavigationView navigationView;
    NavigationWidget *widget = new NavigationWidget();
    navigationView.widget = widget;
    navigationView.dockToolBarWidgets = widget->createToolButtons();
    return navigationView;
}


/*!
   Returns a settings prefix for \a position.
*/
static QString settingsPrefix(int position)
{
    return QString::fromLatin1("ClassView.Treewidget.%1.FlatMode").arg(position);
}

//! Flat mode settings

void NavigationWidgetFactory::saveSettings(QSettings *settings, int position, QWidget *widget)
{
    NavigationWidget *pw = qobject_cast<NavigationWidget *>(widget);
    QTC_ASSERT(pw, return);

    // .beginGroup is not used - to prevent simultaneous access
    QString settingsGroup = settingsPrefix(position);
    settings->setValue(settingsGroup, pw->flatMode());
}

void NavigationWidgetFactory::restoreSettings(QSettings *settings, int position, QWidget *widget)
{
    NavigationWidget *pw = qobject_cast<NavigationWidget *>(widget);
    QTC_ASSERT(pw, return);

    // .beginGroup is not used - to prevent simultaneous access
    QString settingsGroup = settingsPrefix(position);
    pw->setFlatMode(settings->value(settingsGroup, false).toBool());
}

} // namespace Internal
} // namespace ClassView
