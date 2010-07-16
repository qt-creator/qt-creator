/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
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

#include "classviewplugin.h"
#include "classviewmanager.h"
#include "classviewnavigationwidgetfactory.h"

#include <QtCore/QtPlugin>

namespace ClassView {
namespace Internal {

///////////////////////////////// PluginPrivate //////////////////////////////////
/*!
   \struct PluginPrivate
   \brief Private class data for \a Plugin
   \sa Plugin
 */
struct PluginPrivate
{
    //! Pointer to Navi Widget Factory
    QPointer<NavigationWidgetFactory> navigationWidgetFactory;

    //! Pointer to Manager
    QPointer<Manager> manager;
};

///////////////////////////////// Plugin //////////////////////////////////

Plugin::Plugin()
    : d_ptr(new PluginPrivate())
{
}

Plugin::~Plugin()
{
}

bool Plugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    // create a navigation widget factory
    d_ptr->navigationWidgetFactory = NavigationWidgetFactory::instance();

    // add to ExtensionSystem
    addAutoReleasedObject(d_ptr->navigationWidgetFactory);

    // create manager
    d_ptr->manager = Manager::instance(this);

    return true;
}

void Plugin::extensionsInitialized()
{
}

} // namespace Internal
} // namespace ClassView

Q_EXPORT_PLUGIN(ClassView::Internal::Plugin)
