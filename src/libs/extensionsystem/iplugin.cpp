/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "iplugin.h"
#include "iplugin_p.h"
#include "pluginmanager.h"
#include "pluginspec.h"

/*!
    \class ExtensionSystem::IPlugin
    \mainclass
    \brief Base class for all plugins.

    The IPlugin class is an abstract class that must be implemented
    once for each plugin.
    A plugin consists of two parts: A description file, and a library
    that at least contains the IPlugin implementation.

    \tableofcontents

    \section1 Plugin Specification

    A plugin needs to provide a plugin specification file in addition
    to the actual plugin library, so the plugin manager can find the plugin,
    resolve its dependencies, and load it. For more information,
    see \l{Plugin Specifications}.

    \section1 Plugin Implementation
    Plugins must provide one implementation of the IPlugin class, located
    in a library that matches the \c name attribute given in their
    XML description. The IPlugin implementation must be exported and
    made known to Qt's plugin system, see the Qt Documentation on the
    \l{http://doc.qt.nokia.com/4.7/qtplugin.html#Q_EXPORT_PLUGIN2}
    {Q_EXPORT_PLUGIN2 macro}.

    After the plugins' XML files have been read, and dependencies have been
    found, the plugin loading is done in three phases:
    \list 1
    \o All plugin libraries are loaded in \e{root-to-leaf} order of the
       dependency tree.
    \o All plugins' initialize methods are called in \e{root-to-leaf} order
       of the dependency tree. This is a good place to put
       objects in the plugin manager's object pool.
    \o All plugins' extensionsInitialized methods are called in \e{leaf-to-root}
       order of the dependency tree. At this point, plugins can
       be sure that all plugins that depend on this plugin have
       been initialized completely (implying that they have put
       objects in the object pool, if they want that during the
       initialization sequence).
    \endlist
    If library loading or initialization of a plugin fails, all plugins
    that depend on that plugin also fail.

    Plugins have access to the plugin manager
    (and its object pool) via the PluginManager::instance()
    method.
*/

/*!
    \fn bool IPlugin::initialize(const QStringList &arguments, QString *errorString)
    \brief Called after the plugin has been loaded and the IPlugin instance
    has been created.

    The initialize methods of plugins that depend
    on this plugin are called after the initialize method of this plugin
    has been called. Plugins should initialize their internal state in this
    method. Returns if initialization of successful. If it wasn't successful,
    the \a errorString should be set to a user-readable message
    describing the reason.

    \sa extensionsInitialized()
*/

/*!
    \fn void IPlugin::extensionsInitialized()
    \brief Called after the IPlugin::initialize() method has been called,
    and after both the IPlugin::initialize() and IPlugin::extensionsInitialized()
    methods of plugins that depend on this plugin have been called.

    In this method, the plugin can assume that plugins that depend on
    this plugin are fully 'up and running'. It is a good place to
    look in the plugin manager's object pool for objects that have
    been provided by dependent plugins.

    \sa initialize()
*/

/*!
    \fn IPlugin::ShutdownFlag IPlugin::aboutToShutdown()
    \brief Called during a shutdown sequence in the same order as initialization
    before the plugins get deleted in reverse order.

    This method should be used to disconnect from other plugins,
    hide all UI, and optimize shutdown in general.
    If a plugin needs to delay the real shutdown for a while, for example if
    it needs to wait for external processes to finish for a clean shutdown,
    the plugin can return IPlugin::AsynchronousShutdown from this method. This
    will keep the main event loop running after the aboutToShutdown() sequence
    has finished, until all plugins requesting AsynchronousShutdown have sent
    the asynchronousShutdownFinished() signal.

    The default implementation of this method does nothing and returns
    IPlugin::SynchronousShutdown.

    Returns IPlugin::AsynchronousShutdown if the plugin needs to perform
    asynchronous actions before performing the shutdown.

    \sa asynchronousShutdownFinished()
*/

/*!
    \fn void IPlugin::asynchronousShutdownFinished()
    Sent by the plugin implementation after a asynchronous shutdown
    is ready to proceed with the shutdown sequence.

    \sa aboutToShutdown()
*/

using namespace ExtensionSystem;

/*!
    \fn IPlugin::IPlugin()
    \internal
*/
IPlugin::IPlugin()
    : d(new Internal::IPluginPrivate())
{
}

/*!
    \fn IPlugin::~IPlugin()
    \internal
*/
IPlugin::~IPlugin()
{
    PluginManager *pm = PluginManager::instance();
    foreach (QObject *obj, d->addedObjectsInReverseOrder)
        pm->removeObject(obj);
    qDeleteAll(d->addedObjectsInReverseOrder);
    d->addedObjectsInReverseOrder.clear();
    delete d;
    d = 0;
}

/*!
    \fn PluginSpec *IPlugin::pluginSpec() const
    Returns the PluginSpec corresponding to this plugin.
    This is not available in the constructor.
*/
PluginSpec *IPlugin::pluginSpec() const
{
    return d->pluginSpec;
}

/*!
    \fn void IPlugin::addObject(QObject *obj)
    Convenience method that registers \a obj in the plugin manager's
    plugin pool by just calling PluginManager::addObject().
*/
void IPlugin::addObject(QObject *obj)
{
    PluginManager::instance()->addObject(obj);
}

/*!
    \fn void IPlugin::addAutoReleasedObject(QObject *obj)
    Convenience method for registering \a obj in the plugin manager's
    plugin pool. Usually, registered objects must be removed from
    the object pool and deleted by hand.
    Objects added to the pool via addAutoReleasedObject are automatically
    removed and deleted in reverse order of registration when
    the IPlugin instance is destroyed.
    \sa PluginManager::addObject()
*/
void IPlugin::addAutoReleasedObject(QObject *obj)
{
    d->addedObjectsInReverseOrder.prepend(obj);
    PluginManager::instance()->addObject(obj);
}

/*!
    \fn void IPlugin::removeObject(QObject *obj)
    Convenience method that unregisters \a obj from the plugin manager's
    plugin pool by just calling PluginManager::removeObject().
*/
void IPlugin::removeObject(QObject *obj)
{
    PluginManager::instance()->removeObject(obj);
}

