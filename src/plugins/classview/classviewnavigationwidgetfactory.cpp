/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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

static NavigationWidgetFactory *factoryInstance;

///////////////////////////////// NavigationWidgetFactory //////////////////////////////////

NavigationWidgetFactory::NavigationWidgetFactory()
{
    factoryInstance = this;
}

NavigationWidgetFactory::~NavigationWidgetFactory()
{
    factoryInstance = 0;
}

NavigationWidgetFactory *NavigationWidgetFactory::instance()
{
    return factoryInstance;
}

QString NavigationWidgetFactory::displayName() const
{
    return tr("Class View");
}

int NavigationWidgetFactory::priority() const
{
    return 500;
}

Core::Id NavigationWidgetFactory::id() const
{
    return Core::Id("Class View");
}

QKeySequence NavigationWidgetFactory::activationSequence() const
{
    return QKeySequence();
}

Core::NavigationView NavigationWidgetFactory::createWidget()
{
    Core::NavigationView navigationView;
    NavigationWidget *widget = new NavigationWidget();
    navigationView.widget = widget;
    navigationView.dockToolBarWidgets = widget->createToolButtons();
    emit widgetIsCreated();
    return navigationView;
}


/*!
   \brief Get a settings prefix for the specified position
   \param position Position
   \return Settings prefix
 */
static QString settingsPrefix(int position)
{
    return QString::fromLatin1("ClassView/Treewidget.%1/FlatMode").arg(position);
}

//! Flat mode settings

void NavigationWidgetFactory::saveSettings(int position, QWidget *widget)
{
    NavigationWidget *pw = qobject_cast<NavigationWidget *>(widget);
    QTC_ASSERT(pw, return);

    QSettings *settings = Core::ICore::settings();
    QTC_ASSERT(settings, return);

    // .beginGroup is not used - to prevent simultaneous access
    QString group = settingsPrefix(position);

    // save settings
    settings->setValue(group, pw->flatMode());
}

void NavigationWidgetFactory::restoreSettings(int position, QWidget *widget)
{
    NavigationWidget *pw = qobject_cast<NavigationWidget *>(widget);
    QTC_ASSERT(pw, return);

    QSettings *settings = Core::ICore::settings();
    QTC_ASSERT(settings, return);

    // .beginGroup is not used - to prevent simultaneous access
    QString group = settingsPrefix(position);

    // load settings
    pw->setFlatMode(settings->value(group, false).toBool());
}

} // namespace Internal
} // namespace ClassView
