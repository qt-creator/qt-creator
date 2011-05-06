/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
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
